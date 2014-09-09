// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

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

	int thresh_low = 50;
	int thresh_high = 227;
	int mask_radius = 116;

	Mat frame, body_mask;

	for (int imageCount = 0; imageCount != nframes; imageCount++)
	{
		if (argc == 2)
			frame = f.ReadFrame(imageCount);
		else
		 	frame = wingcam.GrabFrame();

		createTrackbar("Threshold low", "raw image", &thresh_low, 255);
		//createTrackbar("Threshold high", "raw image", &thresh_high, 255);
		//createTrackbar("Mask radius", "raw image", &mask_radius, imageWidth/2);
		
		//threshold(frame, frame, thresh_low, 255, CV_THRESH_TOZERO);
		//threshold(frame, frame, thresh_high, 255, CV_THRESH_TOZERO_INV);

		threshold(frame, body_mask, thresh_low, 255, CV_THRESH_BINARY_INV);

		vector<vector<Point> > contours;
		vector<Vec4i> hierarchy;

		findContours(body_mask, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

		/// Find the rotated rectangles and ellipses for each contour
		vector<RotatedRect> minRect(contours.size());
		vector<RotatedRect> minEllipse(contours.size());

		for (int i = 0; i < contours.size(); i++)
		{
			minRect[i] = minAreaRect(Mat(contours[i]));
			if (contours[i].size() > 5)
			{
				minEllipse[i] = fitEllipse(Mat(contours[i]));
			}
			ellipse(frame, minEllipse[i], Scalar(0,0,0), 1, 8);
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

