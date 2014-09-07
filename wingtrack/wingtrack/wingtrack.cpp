// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

#define BGN 1000

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
		error = wingcam.SetCameraParameters(384, 256, 512, 512);
		error = wingcam.Start();
		
		if (error != PGRERROR_OK)
		{
			error.PrintErrorTrace();
			return -1;
		}
	}

	Mat frame, fgmask, bframe;

	Mat bg = Mat::zeros(Size(imageWidth, imageHeight), CV_32FC1);


	printf("\nComputing background model...\n");
	for(int imageCount = 0; imageCount != BGN; imageCount++)
	{
		if (argc == 2)
			frame = f.ReadFrame(imageCount);
		else
			frame = wingcam.GrabFrame();

		accumulate(frame, bg);
	}

	bg = bg / BGN;
	bg.convertTo(bg, CV_8UC1);

	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;

	for (int imageCount = 0; imageCount != nframes; imageCount++)
	{
		if (argc == 2)
			frame = f.ReadFrame(imageCount);
		else
		 	frame = wingcam.GrabFrame();

		absdiff(frame, bg, fgmask);
		threshold(fgmask, fgmask, 50, 255, CV_THRESH_BINARY);

		findContours(fgmask, contours, hierarchy, CV_RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
		vector<RotatedRect> minEllipse(contours.size());

		for (int i = 0; i < contours.size(); i++)
		{
			drawContours(fgmask, contours, i, Scalar(255, 255, 255), 1, 8, vector<Vec4i>(), 0, Point());
			if (contours[i].size() > 5)
			{
				minEllipse[i] = fitEllipse(Mat(contours[i]));
				ellipse(frame, minEllipse[i], Scalar(255, 255, 255), 1, 1);
			}
		}

		imshow("raw image", frame);
		imshow("background", bg);
		imshow("FG mask", fgmask);

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
