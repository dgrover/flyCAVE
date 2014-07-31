// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "fmfwriter.h"
#include "camera.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

Camera cam;

queue <Image> rawImageStream;
queue <Image> dispImageStream;
queue <TimeStamp> rawTimeStamps;

int _tmain(int argc, _TCHAR* argv[])
{
	FmfWriter fout;

	bool stream = true;
	int success;
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

	// Power on the camera
	const unsigned int k_cameraPower = 0x610;
	const unsigned int k_powerVal_on = 0x80000000;
	error  = cam.WriteRegister( k_cameraPower, k_powerVal_on );
	if (error != PGRERROR_OK)
	{
		PrintError( error );
		return -1;
	}

	const unsigned int millisecondsToSleep = 100;
	unsigned int regVal = 0;
	unsigned int retries = 10;

	// Wait for camera to complete power-up
	do 
	{
#if defined(WIN32) || defined(WIN64)
		Sleep(millisecondsToSleep);    
#else
		usleep(millisecondsToSleep * 1000);
#endif
		error = cam.ReadRegister(k_cameraPower, &regVal);
		if (error == FlyCapture2::PGRERROR_TIMEOUT)
		{
			// ignore timeout errors, camera may not be responding to
			// register reads during power-up
		}
		else if (error != FlyCapture2::PGRERROR_OK)
		{
			PrintError( error );
			return -1;
		}

		retries--;
	} while ((regVal & k_powerVal_on) == 0 && retries > 0);

	// Check for timeout errors after retrying
	if (error == FlyCapture2::PGRERROR_TIMEOUT)
	{
		PrintError( error );
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

	//Lower shutter speed for fast triggering
	FlyCapture2::Property pProp;

	pProp.type = SHUTTER;
	pProp.absControl = true;
	pProp.onePush = false;
	pProp.onOff = true;
	pProp.autoManualMode = false;
	pProp.absValue = 0.006;

	error = cam.SetProperty( &pProp );
    if (error != PGRERROR_OK)
    {
		PrintError( error );
        return -1;
    }

	//Cranking up the gain all the way
	pProp.type = GAIN;
	pProp.absControl = true;
	pProp.onePush = false;
	pProp.onOff = true;
	pProp.autoManualMode = false;
	pProp.absValue = 18.062;

	error = cam.SetProperty( &pProp );
    if (error != PGRERROR_OK)
    {
		PrintError( error );
        return -1;
    }

	success = fout.Open();

	if (!success)
		return -1;

	fout.InitHeader(fmt7ImageSettings.width, fmt7ImageSettings.height);
	fout.WriteHeader();

	// Start capturing images
	error = cam.StartCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	//// Retrieve frame rate property
	//Property frmRate;
	//frmRate.type = FRAME_RATE;
	//error = cam.GetProperty(&frmRate);
	//if (error != PGRERROR_OK)
	//{
	//	PrintError(error);
	//	return -1;
	//}

	//printf("Frame rate: %3.2f fps\n\n", frmRate.absValue);

	printf("Streaming. Press [SPACE] to start recording\n\n");

	#pragma omp parallel sections num_threads(3)
	{
		#pragma omp section
		{
			while (stream)
			{
				Image rawImage, convertedImage, dispImage;

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

				#pragma omp critical
				dispImageStream.push(convertedImage);

				if (fout.record)
				{
					#pragma omp critical
					{
						rawImageStream.push(convertedImage);
						rawTimeStamps.push(timestamp);
					}
				}
				
				if (GetAsyncKeyState(VK_SPACE))			//press [SPACE] to start recording
					fout.record = true;

				if (GetAsyncKeyState(VK_ESCAPE))		//press [ESC] to exit
					stream = false;

			}
		}

		#pragma omp section
		{
			while (stream || !rawImageStream.empty())
			{
				printf("Recording buffer size %d, Frames written %d\r", rawImageStream.size(), fout.nframes);

				if (!rawImageStream.empty())
				{
					TimeStamp tStamp = rawTimeStamps.front();
					Image tImage = rawImageStream.front();
					
					fout.WriteFrame(tStamp, tImage);
					fout.WriteLog(tStamp);

					fout.nframes++;

					#pragma omp critical
					{
						rawImageStream.pop();
						rawTimeStamps.pop();
					}
				}
			}
		}

		#pragma omp section
		{
			while (stream)
			{
				if (!dispImageStream.empty())
				{
					Image dImage;
					dImage.DeepCopy(&dispImageStream.back());
					
					// convert to OpenCV Mat
					unsigned int rowBytes = (double)dImage.GetReceivedDataSize() / (double)dImage.GetRows();
					Mat frame = Mat(dImage.GetRows(), dImage.GetCols(), CV_8UC1, dImage.GetData(), rowBytes);		
						
					int width=frame.size().width;
					int height=frame.size().height;

					line(frame, Point((width/2)-50,height/2), Point((width/2)+50, height/2), 255);  //crosshair horizontal
					line(frame, Point(width/2,(height/2)-50), Point(width/2,(height/2)+50), 255);  //crosshair vertical

					imshow("raw image", frame);
					waitKey(1);
					
					#pragma omp critical
					dispImageStream = queue<Image>();
				}
			}
		}
	}

	// Stop capturing images
	error = cam.StopCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Power off the camera
	const unsigned int k_powerVal_off = 0x00000000;
	error  = cam.WriteRegister( k_cameraPower, k_powerVal_off );
	if (error != PGRERROR_OK)
	{
		PrintError( error );
		return -1;
	}

	// Disconnect the camera
	error = cam.Disconnect();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	success = fout.Close();
	
	printf("\n\nDone! Press Enter to exit...\n");
	getchar();

	return 0;
}