// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FlyCapture2.h"

#include <omp.h>
#include <queue>

//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace FlyCapture2;
//using namespace cv;

Camera cam;
FILE *FMF_Out;
FILE *flog;

unsigned __int32 fmfVersion, SizeY, SizeX;
unsigned __int64 bytesPerChunk, nframes;
char *buf;

queue <Image> rawImageStream;
queue <TimeStamp> rawTimeStamps;

char fname[100];
char flogname[100];

void PrintBuildInfo()
{
	FC2Version fc2Version;
	Utilities::GetLibraryVersion(&fc2Version);
	char version[128];
	sprintf_s(
		version,
		"FlyCapture2 library version: %d.%d.%d.%d\n",
		fc2Version.major, fc2Version.minor, fc2Version.type, fc2Version.build);

	printf("%s", version);

	char timeStamp[512];
	sprintf_s(timeStamp, "Application build date: %s %s\n\n", __DATE__, __TIME__);

	printf("%s", timeStamp);
}

void PrintCameraInfo(CameraInfo* pCamInfo)
{
	printf(
		"\n*** CAMERA INFORMATION ***\n"
		"Serial number - %u\n"
		"Camera model - %s\n"
		"Camera vendor - %s\n"
		"Sensor - %s\n"
		"Resolution - %s\n"
		"Firmware version - %s\n"
		"Firmware build time - %s\n\n",
		pCamInfo->serialNumber,
		pCamInfo->modelName,
		pCamInfo->vendorName,
		pCamInfo->sensorInfo,
		pCamInfo->sensorResolution,
		pCamInfo->firmwareVersion,
		pCamInfo->firmwareBuildTime);
}

void PrintFormat7Capabilities(Format7Info fmt7Info)
{
	printf(
		"Max image pixels: (%u, %u)\n"
		"Image Unit size: (%u, %u)\n"
		"Offset Unit size: (%u, %u)\n"
		"Pixel format bitfield: 0x%08x\n",
		fmt7Info.maxWidth,
		fmt7Info.maxHeight,
		fmt7Info.imageHStepSize,
		fmt7Info.imageVStepSize,
		fmt7Info.offsetHStepSize,
		fmt7Info.offsetVStepSize,
		fmt7Info.pixelFormatBitField);
}

void PrintError(Error error)
{
	error.PrintErrorTrace();
}

int _tmain(int argc, _TCHAR* argv[])
{
	bool stream = true;
	bool record = false;

	FMF_Out = new FILE;
	flog = new FILE;

	SYSTEMTIME st;
	GetLocalTime(&st);

	Error error;

	const Mode k_fmt7Mode = MODE_0;
	const PixelFormat k_fmt7PixFmt = PIXEL_FORMAT_RAW8;

	BusManager busMgr;
	unsigned int numCameras;
	error = busMgr.GetNumOfCameras(&numCameras);
	
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	printf("Number of cameras detected: %u\n", numCameras);

	if (numCameras < 1)
	{
		printf("Insufficient number of cameras... exiting\n");
		return -1;
	}

	PGRGuid guid;
	error = busMgr.GetCameraFromIndex(0, &guid);

	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Connect to a camera
	error = cam.Connect(&guid);
	
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Get the camera information
	CameraInfo camInfo;
	error = cam.GetCameraInfo(&camInfo);
	
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	PrintCameraInfo(&camInfo);

	// Query for available Format 7 modes
	Format7Info fmt7Info;
	bool supported;
	fmt7Info.mode = k_fmt7Mode;
	error = cam.GetFormat7Info(&fmt7Info, &supported);
	
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	PrintFormat7Capabilities(fmt7Info);

	if ((k_fmt7PixFmt & fmt7Info.pixelFormatBitField) == 0)
	{
		// Pixel format not supported!
		printf("Pixel format is not supported\n");
		return -1;
	}

	Format7ImageSettings fmt7ImageSettings;
	fmt7ImageSettings.mode = k_fmt7Mode;
	fmt7ImageSettings.offsetX = 0;
	fmt7ImageSettings.offsetY = 0;
	fmt7ImageSettings.width = fmt7Info.maxWidth;
	fmt7ImageSettings.height = fmt7Info.maxHeight;
	fmt7ImageSettings.pixelFormat = k_fmt7PixFmt;

	bool valid;
	Format7PacketInfo fmt7PacketInfo;

	// Validate the settings to make sure that they are valid
	error = cam.ValidateFormat7Settings(
		&fmt7ImageSettings,
		&valid,
		&fmt7PacketInfo);
		
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	if (!valid)
	{
		// Settings are not valid
		printf("Format7 settings are not valid\n");
		return -1;
	}

	// Set the settings to the camera
	error = cam.SetFormat7Configuration(
		&fmt7ImageSettings,
		fmt7PacketInfo.recommendedBytesPerPacket);
	
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	//settings for version 1.0 fmf header
	fmfVersion = 1;
	SizeY = fmt7ImageSettings.height;
	SizeX = fmt7ImageSettings.width;
	bytesPerChunk = SizeY*SizeX + sizeof(double);
	nframes = 0;

	sprintf_s(fname, "E:\\WingCam-%d%02d%02dT%02d%02d%02d.fmf", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(fname);
		
	FMF_Out = fopen(fname, "wb");
		
	if(FMF_Out == NULL)
	{
		printf("\nError opening FMF writer. Recording terminated.");
		return -1;
	}
		
	//write FMF header data
	fwrite(&fmfVersion, sizeof(unsigned __int32), 1, FMF_Out);
	fwrite(&SizeY, sizeof(unsigned __int32), 1, FMF_Out);
	fwrite(&SizeX, sizeof(unsigned __int32), 1, FMF_Out);
	fwrite(&bytesPerChunk, sizeof(unsigned __int64), 1, FMF_Out);
	fwrite(&nframes, sizeof(unsigned __int64), 1, FMF_Out);

	sprintf_s(flogname, "E:\\WingLog-%d%02d%02dT%02d%02d%02d.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(flogname);
		
	flog = fopen(flogname, "w");
		
	if(flog == NULL)
	{
		printf("\nError creating log file. Recording terminated.");
		return -1;
	}

	// Start capturing images
	error = cam.StartCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Retrieve frame rate property
	Property frmRate;
	frmRate.type = FRAME_RATE;
	error = cam.GetProperty(&frmRate);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	printf("\nFrame rate %3.2f fps ...\n\n", frmRate.absValue);

	printf("Streaming. Press [SPACE] to start recording\n\n");

	//namedWindow( "raw image", WINDOW_AUTOSIZE );

	#pragma omp parallel sections num_threads(2)
	{
		#pragma omp section
		{
			while (stream)
			{
				Image rawImage, convertedImage;

				// Retrieve an image
				error = cam.RetrieveBuffer(&rawImage);
				if (error != PGRERROR_OK)
				{
					PrintError(error);
					break;
				}

				//get image timestamp
				TimeStamp timestamp = rawImage.GetTimeStamp();

				// Convert the raw image
				error = rawImage.Convert(PIXEL_FORMAT_MONO8, &convertedImage);
					
				if (error != PGRERROR_OK)
				{
					PrintError(error);
					break;
				}

				if (record)
				{
					#pragma omp critical
					{
						rawImageStream.push(convertedImage);
						rawTimeStamps.push(timestamp);
					}
				}
				
				if (GetAsyncKeyState(VK_SPACE))			//press [SPACE] to start recording
					record = true;

				if (GetAsyncKeyState(VK_ESCAPE))		//press [ESC] to exit
					stream = false;

			}
		}

		#pragma omp section
		{
			while (stream || !rawImageStream.empty())
			{
				printf("Recording buffer size %d, Frames written %d\r", rawImageStream.size(), nframes);

				if (!rawImageStream.empty())
				{
					Image tImage = rawImageStream.front();
					TimeStamp tStamp = rawTimeStamps.front();

					double dtStamp = (double) tStamp.seconds;

					fwrite(&dtStamp, sizeof(double), 1, FMF_Out);
					fwrite(tImage.GetData(), tImage.GetDataSize(), 1, FMF_Out);

					fprintf(flog, "Frame %d - TimeStamp [%d %d]\n", nframes, tStamp.seconds, tStamp.microSeconds);

					nframes++;

					#pragma omp critical
					{
						rawImageStream.pop();
						rawTimeStamps.pop();
					}
				}
			}
		}
	}

	//#pragma omp section
	//{
	//	while (stream)
	//	{
	//		if (!dispImageStream.empty())
	//		{
	//			Image dImage = dispImageStream.front();
	//			// convert to OpenCV Mat
	//			unsigned int rowBytes = (double)dImage.GetReceivedDataSize() / (double)dImage.GetRows();
	//			Mat frame = Mat(dImage.GetRows(), dImage.GetCols(), CV_8UC1, dImage.GetData(), rowBytes);		
	//				
	//			int width=frame.size().width;
	//			int height=frame.size().height;

	//			line(frame, Point((width/2)-50,height/2), Point((width/2)+50, height/2), 255);  //crosshair horizontal
	//			line(frame, Point(width/2,(height/2)-50), Point(width/2,(height/2)+50), 255);  //crosshair vertical

	//			//imshow("raw image", frame);
	//			//waitKey(1);
	//			dispImageStream.pop();
	//		}
	//	}
	//}

	// Stop capturing images
	error = cam.StopCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Disconnect the camera
	error = cam.Disconnect();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	printf( "\nFinished writing %d images\n", nframes );

	//seek to location in file where nframes is stored and replace
	fseek (FMF_Out, 20, SEEK_SET );	
	fwrite(&nframes, sizeof(unsigned __int64), 1, FMF_Out);

	// close fmf file
	fclose(FMF_Out);

	//close log file
	fclose(flog);
	
	if (!record)
	{
		remove(fname);
		remove(flogname);
	}
	
	printf("\nDone! Press Enter to exit...\n");
	getchar();

	return 0;
}

//opencv_core249.lib opencv_highgui249.lib