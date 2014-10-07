// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

bool record = false;
bool stream = true;

queue <Image> rawImageStream;
queue <TimeStamp> rawTimeStamps;
queue <Image> dispImageStream;

int _tmain(int argc, _TCHAR* argv[])
{
	FmfWriter f;
	PGRcam wingcam;
	
	BusManager busMgr;
	unsigned int numCameras;
	PGRGuid guid;

	FlyCapture2::Error error;

	int imageWidth, imageHeight;

	error = busMgr.GetNumOfCameras(&numCameras);
	printf("Number of cameras detected: %u\n", numCameras);

	if (numCameras < 1)
	{
		printf("Insufficient number of cameras... exiting\n");
		return -1;
	}

	//Initialize camera
	error = busMgr.GetCameraFromIndex(0, &guid);
	error = wingcam.Connect(guid);
	error = wingcam.SetCameraParameters(288, 200);
	wingcam.GetImageSize(imageWidth, imageHeight);
	error = wingcam.Start();

	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	f.Open();
	f.InitHeader(imageWidth, imageHeight);
	f.WriteHeader();
	
	printf("Streaming. Press [SPACE] to start recording\n\n");

	#pragma omp parallel sections num_threads(3)
	{
		#pragma omp section
		{
			while (true)
			{
				Image tImage = wingcam.GrabFrame();
				TimeStamp tStamp = wingcam.GetTimeStamp();
								
				#pragma omp critical
				{
					rawImageStream.push(tImage);
					rawTimeStamps.push(tStamp);
					dispImageStream.push(tImage);
				}
				
				if (GetAsyncKeyState(VK_SPACE))			//press [SPACE] to start recording
					record = true;

				if (GetAsyncKeyState(VK_ESCAPE))
				{
					stream = false;
					break;
				}

			}
		}

		#pragma omp section
		{
			while (true)
			{
				if (!rawImageStream.empty())
				{
					if (record)
					{
						Image wImage = rawImageStream.front();
						TimeStamp wStamp = rawTimeStamps.front();
						
						f.WriteFrame(wStamp, wImage);
						f.WriteLog(wStamp);
						f.nframes++;
					}
					
					#pragma omp critical
					{
						rawImageStream.pop();
						rawTimeStamps.pop();
					}
				}
				
				printf("Recording buffer size %d, Frames written %d\r", rawImageStream.size(), f.nframes);

				if (rawImageStream.size() == 0 && !stream)
					break;
			}
		}

		#pragma omp section
		{
			while (true)
			{
				if (!dispImageStream.empty())
				{
					Image dImage;
					dImage.DeepCopy(&dispImageStream.back());
					
					// convert to OpenCV Mat
					Mat frame = wingcam.convertImagetoMat(dImage);
						
					line(frame, Point((imageWidth/2)-50, imageHeight/2), Point((imageWidth/2)+50, imageHeight/2), 255);  //crosshair horizontal
					line(frame, Point(imageWidth/2,(imageHeight/2)-50), Point(imageWidth/2,(imageHeight/2)+50), 255);  //crosshair vertical

					imshow("raw image", frame);
					waitKey(1);
					
					#pragma omp critical
					dispImageStream = queue<Image>();
				}

				if (!stream)
				{
					destroyWindow("raw image");
					break;
				}
			}
		}
	}

	wingcam.Stop();

	f.Close();
	
	printf("\n\nDone! Press Enter to exit...\n");
	getchar();

	return 0;
}