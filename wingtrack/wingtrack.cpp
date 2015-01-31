// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

struct {
	bool operator() (cv::Point pt1, cv::Point pt2) { return (pt1.y < pt2.y); }
} mycomp;

bool stream = true;
bool track = false;
bool record = false;

queue <Mat> dispStream;
queue <Mat> maskStream;

queue <Image> imageStream;
queue <TimeStamp> timeStamps;

queue <float> leftwba;
queue <float> rightwba;

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
		return CV_PI;
	else
		return acos(a) * 180 / CV_PI;
}

int _tmain(int argc, _TCHAR* argv[])
{
	int imageWidth = 256, imageHeight = 256;

	PGRcam wingcam;

	BusManager busMgr;
	unsigned int numCameras;
	PGRGuid guid;

	FlyCapture2::Error error;

	//FmfReader fin;
	FmfWriter fout;

	//fin.Open(argv[1]);
	//fin.ReadHeader();
	//fin.GetImageSize(imageWidth, imageHeight);
	//nframes = fin.GetFrameCount();	

	error = busMgr.GetNumOfCameras(&numCameras);
	//printf("Number of cameras detected: %u\n", numCameras);

	if (numCameras < 1)
	{
		printf("Camera not connected... exiting\n");
		return -1;
	}

	//Initialize camera
	printf("Initializing camera ");
	error = busMgr.GetCameraFromIndex(0, &guid);
	error = wingcam.Connect(guid);
	error = wingcam.SetCameraParameters(imageWidth, imageHeight);
	//wingcam.GetImageSize(imageWidth, imageHeight);
	error = wingcam.SetProperty(SHUTTER, 4.925);
	error = wingcam.SetProperty(GAIN, 0.0);
	error = wingcam.Start();

	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	printf("[OK]\n");

	FlyCapture2::Image img;
	FlyCapture2::TimeStamp stamp;

	Mat frame, mask, fly_blob, body_mask;

	int thresh = 190;
	int body_thresh = 150;

	Mat erodeElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	Mat dilateElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));

	Point2f center(imageWidth / 2, imageHeight / 2);
	int key_state = 0;

	float left_angle, right_angle;
	int count = 0;

	printf("Press [F1] to start/stop recording. Press [ESC] to exit.\n\n");

	#pragma omp parallel sections num_threads(3)
	{
		#pragma omp section
		{
			int ltime = 0;
			int ctime = 0;
			int dtime = 0;

			while (true)
			{
				//frame = fin.ReadFrame(imageCount);

				img = wingcam.GrabFrame();
				stamp = wingcam.GetTimeStamp();
				frame = wingcam.convertImagetoMat(img);

				threshold(frame, body_mask, body_thresh, 255, THRESH_BINARY_INV);
				threshold(frame, mask, thresh, 255, THRESH_BINARY_INV);

				dilate(body_mask, body_mask, dilateElement, Point(-1, -1), 3);
				body_mask = Scalar::all(255) - body_mask;

				mask &= body_mask;

				left_angle = 0;
				right_angle = 0;

				if (track)
				{
					erode(mask, mask, erodeElement, Point(-1, -1), 3);
					dilate(mask, mask, dilateElement, Point(-1, -1), 3);

					// Find contours
					vector<vector<Point>> contours;
					findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

					vector<vector<Point>> hull(contours.size());

					for (int i = 0; i < contours.size(); i++)
					{
						if (contours[i].size() > 100)
						{
							convexHull(Mat(contours[i]), hull[i], false);
							
							drawContours(frame, contours, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());
							drawContours(frame, hull, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());

							std::sort(hull[i].begin(), hull[i].end(), mycomp);

							line(frame, hull[i].front(), center, Scalar(255, 255, 255), 1, LINE_AA);
							line(frame, hull[i].back(), center, Scalar(255, 255, 255), 1, LINE_AA);
							
							if (hull[i].front().x < center.x)
								left_angle = angleBetween(hull[i].front(), hull[i].back(), center);
							else
								right_angle = angleBetween(hull[i].front(), hull[i].back(), center);


							
						}
					}
				}

				ctime = stamp.cycleCount;

				if (ctime < ltime)
					dtime = ctime + (8000 - ltime);
				else
					dtime = ctime - ltime;

				if (dtime > 0)
					dtime = 8000 / dtime;
				else
					dtime = 0;

				ltime = ctime;

				putText(frame, to_string(dtime), Point(225, 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));

				if (record)
					putText(frame, to_string(count++), Point(0, 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
				
				#pragma omp critical
				{
					maskStream.push(mask);
					dispStream.push(frame);

					leftwba.push(left_angle);
					rightwba.push(right_angle);

					if (record)
					{
						timeStamps.push(stamp);
						imageStream.push(img);
					}
				}

				if (GetAsyncKeyState(VK_F1))
				{
					if (!key_state)
					{
						record = !record;
						count = 0;
					}

					key_state = 1;
				}
				else
					key_state = 0;

				if (GetAsyncKeyState(VK_RETURN))
					track = true;

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
					if (!fout.IsOpen())
					{
						fout.Open();
						fout.InitHeader(imageWidth, imageHeight);
						fout.WriteHeader();
						printf("Recording ");
					}

					fout.WriteFrame(timeStamps.front(), imageStream.front());
					fout.WriteLog(timeStamps.front());
					fout.WriteWBA(leftwba.front(), rightwba.front());
					fout.nframes++;
				
					#pragma omp critical
					{
						imageStream.pop();
						timeStamps.pop();
						leftwba.pop();
						rightwba.pop();
					}
				}
				else
				{
					if (!record && fout.IsOpen())
					{
						fout.Close();
						printf("[OK]\n");
					}
				}

				if (!stream)
				{
					if (imageStream.empty())
					{
						if (record)
						{
							fout.Close();
							printf("[OK]\n");
						}
						break;
					}
				}
			}
		}

		#pragma omp section
		{
			namedWindow("controls", WINDOW_AUTOSIZE);
			createTrackbar("thresh", "controls", &thresh, 255);
			createTrackbar("body thresh", "controls", &body_thresh, 255);
			
			while (true)
			{
				if (!dispStream.empty())
				{
					line(dispStream.back(), Point((imageWidth / 2) - 10, imageHeight / 2), Point((imageWidth / 2) + 10, imageHeight / 2), 255);  //crosshair horizontal
					line(dispStream.back(), Point(imageWidth / 2, (imageHeight / 2) - 10), Point(imageWidth / 2, (imageHeight / 2) + 10), 255);  //crosshair vertical

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

	return 0;
}