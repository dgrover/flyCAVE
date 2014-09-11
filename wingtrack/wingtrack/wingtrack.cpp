// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

#define PI 3.14159265358979323846
#define N 50

float angleBetween(Point v1, Point v2, Point c)
{
	v1 = v1 - c;
	v2 = v2 - c;

	float len1 = sqrt(v1.x * v1.x + v1.y * v1.y);
	float len2 = sqrt(v2.x * v2.x + v2.y * v2.y);

	float dot = v1.x * v2.x + v1.y * v2.y;

	float a = dot / (len1 * len2);

	if (a >= 1.0)
		return 0.0;
	else if (a <= -1.0)
		return PI;
	else
		return acos(a)*180/PI;
}

int _tmain(int argc, _TCHAR* argv[])
{
	Flycam wingcam;

	BusManager busMgr;
	unsigned int numCameras;
	PGRGuid guid;

	FlyCapture2::Error error;

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

	Mat frame, fg, bg, body_mask, fly_blob, outer_mask;

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

	int idx;

	threshold(bg, body_mask, 100, 255, THRESH_BINARY);
	threshold(bg, fly_blob, 100, 255, THRESH_BINARY_INV);

	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(fly_blob, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	/// Find the rotated rectangles and ellipses for each contour
	vector<RotatedRect> minEllipse(contours.size());

	for (int i = 0; i < contours.size(); i++)
	{
		if (contours[i].size() > 50)
		{
			minEllipse[i] = fitEllipse(Mat(contours[i]));
			ellipse(fly_blob, minEllipse[i], Scalar::all(255), 1, 8);

			idx = i;
		}
	}

	outer_mask = Mat::zeros(Size(imageWidth, imageHeight), CV_8UC1);
	circle(outer_mask, minEllipse[idx].center, imageWidth / 2, Scalar(255, 255, 255), FILLED);

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
		threshold(fg, fg, 50, 255, THRESH_BINARY);
		
		fg &= outer_mask;

		erode(fg, fg, erodeElement, Point(-1, -1), 3);
		dilate(fg, fg, dilateElement, Point(-1, -1), 3);

		// Find contours
		std::vector<std::vector<cv::Point> > contours;
		cv::findContours(fg, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		vector<Point2f> triangle;
		float left_angle, right_angle;

		// Find the convex hull object for each contour
		//vector<vector<Point> >hull(contours.size());
		for (int i = 0; i < contours.size(); i++)
		{
			if (contours[i].size() > 50)
			{
				//drawContours(fg, contours, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());
				
				// Find the minimum area enclosing triangle
				minEnclosingTriangle(contours[i], triangle);

				// Draw the triangle
				if (triangle.size() > 0)
				{
					double min_dist = norm(minEllipse[idx].center);
					int min_idx = -1;
					
					for (int j = 0; j < 3; j++)
					{
						double dist = norm(triangle[j] - minEllipse[idx].center);
						if (dist < min_dist)
						{
							min_dist = dist;
							min_idx = j;
						}
					}

					triangle.erase(triangle.begin() + min_idx);

					if (triangle[0].x < minEllipse[idx].center.x)
						left_angle = angleBetween(triangle[0], triangle[1], minEllipse[idx].center);
					else
						right_angle = angleBetween(triangle[0], triangle[1], minEllipse[idx].center);

					for (int j = 0; j < 2; j++)
						line(frame, triangle[j], minEllipse[idx].center, Scalar(255, 255, 255), 1, LINE_AA);

				}
			}
		}

		printf("%f %f\n", left_angle, right_angle);

		imshow("raw image", frame);
		//imshow("foreground mask", fg);

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
