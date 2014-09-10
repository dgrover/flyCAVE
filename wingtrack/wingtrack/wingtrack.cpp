// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

#define N 50

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

	Mat frame, fg, bg, body_mask, outer_mask;

	printf("\nComputing background model... ");
	bg = Mat::zeros(Size(imageWidth, imageHeight), CV_32FC1);
	
	for (int imageCount = 0; imageCount != N; imageCount++)
	{
		if (argc == 2)
			frame = f.ReadFrame(imageCount);
		else
			frame = wingcam.GrabFrame();

		accumulate(frame, bg);
	}

	bg = bg / N;
	bg.convertTo(bg, CV_8UC1);

	printf("Done\n");

	threshold(bg, body_mask, 100, 255, CV_THRESH_BINARY);

	outer_mask = Mat::zeros(Size(imageWidth, imageHeight), CV_8UC1);
	circle(outer_mask, Point(imageWidth / 2, imageHeight / 2), imageWidth / 2, Scalar(255, 255, 255), CV_FILLED);


	Mat erodeElement = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
	Mat dilateElement = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));

	for (int imageCount = 0; imageCount != nframes; imageCount++)
	{
		if (argc == 2)
			frame = f.ReadFrame(imageCount);
		else
			frame = wingcam.GrabFrame();

		//apply body mask to frame and bg
		frame &= body_mask;
		
		absdiff(frame, body_mask, fg);
		threshold(fg, fg, 50, 255, CV_THRESH_BINARY);
		
		fg &= outer_mask;

		erode(fg, fg, erodeElement, Point(-1, -1), 2);
		dilate(fg, fg, dilateElement, Point(-1, -1), 2);

		// Find contours
		std::vector<std::vector<cv::Point> > contours;
		cv::findContours(fg, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

		// Find the convex hull object for each contour
		vector<vector<Point> >hull(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			if (contours[i].size() > 50)
			{
				drawContours(fg, contours, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());
				convexHull(Mat(contours[i]), hull[i], false);
				drawContours(frame, hull, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());
			}
		}
		
		imshow("raw image", frame);
		imshow("foreground mask", fg);

		waitKey(1);

		if (GetAsyncKeyState(VK_ESCAPE))
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

