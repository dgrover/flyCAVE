// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

#define PI 3.14159265358979323846
#define N 50

bool stream = true;
bool record = false;

queue <Mat> dispStream;
queue <Mat> maskStream;

queue <Image> imageStream;
queue <TimeStamp> timeStamps;

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
	PGRcam wingcam;

	BusManager busMgr;
	unsigned int numCameras;
	PGRGuid guid;

	FlyCapture2::Error error;

	//FmfReader fin;
	FmfWriter fout;

	int imageWidth = 288, imageHeight = 200;
	
	//fin.Open(argv[1]);
	//fin.ReadHeader();
	//fin.GetImageSize(imageWidth, imageHeight);
	//nframes = fin.GetFrameCount();	

	error = busMgr.GetNumOfCameras(&numCameras);
	//printf("Number of cameras detected: %u\n", numCameras);

	if (numCameras < 1)
	{
		printf("Insufficient number of cameras... exiting\n");
		return -1;
	}

	//Initialize camera
	error = busMgr.GetCameraFromIndex(0, &guid);
	error = wingcam.Connect(guid);
	error = wingcam.SetCameraParameters(imageWidth, imageHeight);
	//wingcam.GetImageSize(imageWidth, imageHeight);
	error = wingcam.Start();
		
	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	FlyCapture2::Image img;
	FlyCapture2::TimeStamp stamp;

	Mat frame, mask, fly_blob, body_mask;

	fout.Open();
	fout.InitHeader(imageWidth, imageHeight);
	fout.WriteHeader();

	int radius = 100;
	
	Mat erodeElement = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
	Mat dilateElement = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));

	#pragma omp parallel sections num_threads(3)
	{
		#pragma omp section
		{
			while (true)
			{
				int idx = -1;
				//frame = fin.ReadFrame(imageCount);
				
				img = wingcam.GrabFrame();
				stamp = wingcam.GetTimeStamp();
				frame = wingcam.convertImagetoMat(img);
				
				threshold(frame, fly_blob, 25, 255, THRESH_BINARY_INV);
				threshold(frame, body_mask, 100, 255, THRESH_BINARY_INV);
				threshold(frame, mask, 150, 255, THRESH_BINARY_INV);

				dilate(body_mask, body_mask, dilateElement, Point(-1, -1), 2);
				body_mask = Scalar::all(255) - body_mask;

				vector<vector<Point>> contours;
				findContours(fly_blob, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

				/// Find the rotated rectangles and ellipses for each contour
				vector<RotatedRect> minEllipse(contours.size());

				for (int i = 0; i < contours.size(); i++)
				{
					if (contours[i].size() > 50)
					{
						minEllipse[i] = fitEllipse(Mat(contours[i]));
						ellipse(frame, minEllipse[i], Scalar::all(255), 1, 8);

						idx = i;
					}
				}

				//Mat outer_mask = Mat::zeros(Size(imageWidth, imageHeight), CV_8UC1);
				//circle(outer_mask, minEllipse[idx].center, radius, Scalar(255, 255, 255), FILLED);

				//threshold(frame, mask, 150, 255, THRESH_BINARY_INV);
				
				mask &= body_mask;
				//mask &= outer_mask;

				//erode(mask, mask, erodeElement, Point(-1, -1), 3);
				//dilate(mask, mask, dilateElement, Point(-1, -1), 3);

				//// Find contours
				////std::vector<std::vector<cv::Point> > contours;
				//cv::findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

				//vector<Point2f> triangle;
				//float left_angle, right_angle;

				//// Find the convex hull object for each contour
				////vector<vector<Point> >hull(contours.size());
				//for (int i = 0; i < contours.size(); i++)
				//{
				//	if (contours[i].size() > 50)
				//	{
				//		drawContours(mask, contours, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());

				//		// Find the minimum area enclosing triangle
				//		minEnclosingTriangle(contours[i], triangle);

				//		// Draw the triangle
				//		if (triangle.size() > 0)
				//		{
				//			double min_dist = norm(minEllipse[idx].center);
				//			int min_idx = -1;

				//			for (int j = 0; j < 3; j++)
				//			{
				//				double dist = norm(triangle[j] - minEllipse[idx].center);
				//				if (dist < min_dist)
				//				{
				//					min_dist = dist;
				//					min_idx = j;
				//				}
				//			}

				//			triangle.erase(triangle.begin() + min_idx);

				//			if (triangle[0].x < minEllipse[idx].center.x)
				//				left_angle = angleBetween(triangle[0], triangle[1], minEllipse[idx].center);
				//			else
				//				right_angle = angleBetween(triangle[0], triangle[1], minEllipse[idx].center);

				//			for (int j = 0; j < 2; j++)
				//				line(frame, triangle[j], minEllipse[idx].center, Scalar(255, 255, 255), 1, LINE_AA);

				//		}
				//	}
				//}

				#pragma omp critical
				{
					dispStream.push(frame);
					maskStream.push(mask);
										
					imageStream.push(img);
					timeStamps.push(stamp);
				}

				//printf("%f %f\n", left_angle, right_angle);

				if (GetAsyncKeyState(VK_SPACE))
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
				if (!imageStream.empty())
				{
					if (record)
					{
						Image wImage = imageStream.front();
						TimeStamp wStamp = timeStamps.front();

						fout.WriteFrame(wStamp, wImage);
						fout.WriteLog(wStamp);
						fout.nframes++;
					}

					#pragma omp critical
					{
						imageStream.pop();
						timeStamps.pop();
					}
				}

				printf("Recording buffer size %d, Frames written %d\r", imageStream.size(), fout.nframes);

				if (imageStream.size() == 0 && !stream)
					break;
			}
		}

		#pragma omp section
		{
			namedWindow("controls");
			createTrackbar("radius", "controls", &radius, imageWidth/2);
			
			while (true)
			{
				if (!dispStream.empty())
				{
					imshow("image", dispStream.back());
					imshow("mask", maskStream.back());
					
					#pragma omp critical
					{
						dispStream = queue<Mat>();
						maskStream = queue<Mat>();
					}
				}

				waitKey(1);

				if (!stream)
				{
					destroyWindow("image");
					destroyWindow("mask");
					break;
				}
			}
		}

	}

	//fin.Close();
	wingcam.Stop();

	fout.Close();

	printf("\n\nPress Enter to exit...\n");
	getchar();

	return 0;
}
