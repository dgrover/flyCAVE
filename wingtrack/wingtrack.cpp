// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FlyCapture2.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/gpu/gpu.hpp>

using namespace FlyCapture2;
using namespace cv;
using namespace cv::gpu;

#define USEGPU 1

FILE *FMF_In;
unsigned __int32 fmfVersion, SizeY, SizeX, nframes;
unsigned __int64 bytesPerChunk;
long maxFramesInFile;
char *buf;

Camera cam;

BackgroundSubtractorMOG mog_cpu;
MOG_GPU mog_gpu;

Mat frame, fgmask;
GpuMat d_frame, d_fgmask; 

Error error;

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

void ReadFMFHeader()
{		
	fread(&fmfVersion, sizeof(unsigned __int32), 1, FMF_In);
	fread(&SizeY, sizeof(unsigned __int32), 1, FMF_In);
	fread(&SizeX, sizeof(unsigned __int32), 1, FMF_In);
	fread(&bytesPerChunk, sizeof(unsigned __int64), 1, FMF_In);
	fread(&nframes, sizeof(unsigned __int64), 1, FMF_In);

	buf = new char[bytesPerChunk];
			
	maxFramesInFile = (unsigned long int)nframes;
}


int ReadFMFFrame(unsigned long frameIndex)
{
	//IplImage *temp = cvCreateImage(cvSize(SizeX, SizeY), IPL_DEPTH_8U, 1);
	
	if((long)frameIndex>=0L && (long)frameIndex < maxFramesInFile)
		fseek (FMF_In, frameIndex*bytesPerChunk + 28 , SEEK_SET );
	else
		return -1; // Cannot grab .. illegal frame number

	fread(buf, sizeof(double), 1, FMF_In);
	fread(buf, bytesPerChunk-sizeof(double), 1, FMF_In);
	
	//memmove(temp->imageData, buf, bytesPerChunk);	
	return 1;	
}

void CloseFMF()
{
	fclose(FMF_In);
}

int _tmain(int argc, _TCHAR* argv[])
{
	bool isVideo = false;
	bool isCam = false;

	if (argc == 2)
	{
		FMF_In = fopen(argv[1], "rb");
	
		if(FMF_In == NULL) // Cannot open File
		{
			printf("Cannot open input video file\n");
			return -1;	
		}
		
		ReadFMFHeader();
		isVideo = true;				
	}
	else
	{
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

		printf("Frame rate is %3.2f fps\n", frmRate.absValue);
		
		isCam = true;
			
	}

	for (int imageCount = 0; ; imageCount++)
	{
		if (isCam)
		{
			Image rawImage, convertedImage;
			
			// Retrieve an image
			error = cam.RetrieveBuffer(&rawImage);
			if (error != PGRERROR_OK)
			{
				PrintError(error);
				continue;
			}

			// Convert the raw image
			error = rawImage.Convert(PIXEL_FORMAT_MONO8, &convertedImage);
			
			if (error != PGRERROR_OK)
			{
				PrintError(error);
				return -1;
			}

			// convert to OpenCV Mat
			unsigned int rowBytes = (double)convertedImage.GetReceivedDataSize() / (double)convertedImage.GetRows();
			Mat tFrame = Mat(convertedImage.GetRows(), convertedImage.GetCols(), CV_8UC1, convertedImage.GetData(), rowBytes);		
			frame = tFrame;
		
		}
		else if (isVideo)
		{
			int success = ReadFMFFrame(imageCount);
			if (success){
					Mat tFrame = Mat(SizeY, SizeX, CV_8UC1, buf, bytesPerChunk);
					frame = tFrame;
			}
			else
				break;			
		}
		
		if (USEGPU)
		{
			d_frame.upload(frame);
			mog_gpu(d_frame, d_fgmask, 0.01f);
			d_fgmask.download(fgmask);
		}
        else
            mog_cpu(frame, fgmask, 0.01);
		
		imshow("raw image", frame);
		imshow("FG mask", fgmask);

		char key = waitKey(1);
		
		if (key == 27)
			break;
	}

	if (isCam)
	{
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
	}
	else
		CloseFMF();
	
	printf("Done! Press Enter to exit...\n");
	getchar();

	return 0;
}

