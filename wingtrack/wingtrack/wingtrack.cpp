// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

//#define N 1000

int thresh_low = 156;
int thresh_high = 204;

Mat frame, fgmask;

void thresh_callback(int, void*)
{
	threshold(frame, frame, thresh_low, 255, CV_THRESH_TOZERO);
	threshold(frame, frame, thresh_high, 255, CV_THRESH_TOZERO_INV);
};

int _tmain(int argc, _TCHAR* argv[])
{
	Flycam wingcam;

	BusManager busMgr;
	unsigned int numCameras;
	PGRGuid guid;

	Error error;

	FileReader f;

	int nframes = -1;
	int imageWidth, imageHeight;

	if (argc == 2)
	{
		f.Open(argv[1]);
		f.ReadHeader();
		f.GetImageSize(imageWidth, imageHeight);
		nframes = f.GetFrameCount();
	}
	else
	{
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
	}

	//bg = Mat::zeros(Size(imageWidth, imageHeight), CV_32FC1);

	//for (int imageCount = 0; imageCount != N; imageCount++)
	//{
	//	if (argc == 2)
	//		frame = f.ReadFrame(imageCount);
	//	else
	//		frame = wingcam.GrabFrame();

	//	accumulate(frame, bg);
	//}

	//bg = bg / N;
	//bg.convertTo(bg, CV_8UC1);
	

	
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	for (int imageCount = 0; imageCount != nframes; imageCount++)
	{
		if (argc == 2)
			frame = f.ReadFrame(imageCount);
		else
		 	frame = wingcam.GrabFrame();

		createTrackbar("Threshold low", "raw image", &thresh_low, 255, thresh_callback);
		createTrackbar("Threshold high", "raw image", &thresh_high, 255, thresh_callback);
		thresh_callback(0, 0);

		threshold(frame, fgmask, 100, 255, CV_THRESH_BINARY);

		findContours(fgmask, contours, hierarchy, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		vector<RotatedRect> minEllipse(contours.size());

		for (int i = 0; i < contours.size(); i++)
		{
			drawContours(fgmask, contours, i, Scalar(255, 255, 255), 1, 8, vector<Vec4i>(), 0, Point());
			if (contours[i].size() > 100)
			{
				minEllipse[i] = fitEllipse(Mat(contours[i]));
				ellipse(frame, minEllipse[i], Scalar(255, 255, 255), 1, 1);
			}
		}
		
		imshow("raw image", frame);
		
		waitKey(1);
		
		if ( GetAsyncKeyState(VK_ESCAPE) )
			break;
	}

	if (argc == 2)
		f.Close();
	else
		wingcam.Stop();

	printf("\nPress Enter to exit...\n");
	getchar();

	return 0;
}

