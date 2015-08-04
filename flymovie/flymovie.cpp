// flymovie.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;
using namespace moodycamel;

#define MAXRECFRAMES 1000
#define N 1

struct {
	bool operator() (cv::Point pt1, cv::Point pt2) { return (pt1.y < pt2.y); }
} mycomp;

bool stream = true;
bool track = false;
bool record = false;
//bool laser = false;
bool loop = true;

ReaderWriterQueue<Image> q(200);
ReaderWriterQueue<Image> rec(200);
ReaderWriterQueue<Mat> disp_frame(1), disp_mask(1);

ReaderWriterQueue<float> leftwba;
ReaderWriterQueue<float> rightwba;

int last = 0, fps = 0;


int ConvertTimeToFPS(int ctime, int ltime)
{
	int dtime;

	if (ctime < ltime)
		dtime = ctime + (8000 - ltime);
	else
		dtime = ctime - ltime;

	if (dtime > 0)
		dtime = 8000 / dtime;
	else
		dtime = 0;

	return dtime;
}

void OnImageGrabbed(Image* pImage, const void* pCallbackData)
{
	Image img;

	fps = ConvertTimeToFPS(pImage->GetTimeStamp().cycleCount, last);
	last = pImage->GetTimeStamp().cycleCount;

	img.DeepCopy(pImage);
	
	if (stream)
		q.enqueue(img);

	return;
}

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
	//FlyWorld mov("images", "sequence.txt", "..//displaySettings.txt", 912 / 3, 1140 * 2, 1920, 2.0);
	
	FlyWorld mov("..//..//flymetrics//flymovie_patterns//stripe14_images", "..//displaySettings.txt", 912 / 3, 1140 * 2, 1920, 2.0);
	printf("%d images read [OK]\n", mov.numImages);

	int imageWidth = 320, imageHeight = 320;

	PGRcam wingcam;

	BusManager busMgr;
	unsigned int numCameras;
	PGRGuid guid;

	FlyCapture2::Error error;

	FmfWriter fout;

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
	error = wingcam.SetProperty(SHUTTER, 4.887);
	error = wingcam.SetProperty(GAIN, 0.0);
	//error = wingcam.Start();
	error = wingcam.cam.StartCapture(OnImageGrabbed);

	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	printf("[OK]\n");

	//Serial* SP = new Serial("COM5");    // adjust as needed

	//if (SP->IsConnected())
	//	printf("Connecting arduino [OK]\n");

	int thresh = 200;
	int body_thresh = 150;

	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1));

	//Mat erodeElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	//Mat dilateElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));

	Point2f center(imageWidth / 2, imageHeight / 2);

	int track_key_state = 0;
	int record_key_state = 0;
	//int laser_key_state = 0;

	int count = 0;

	//Press [F1] to start/stop recording. Press [ESC] to exit
	#pragma omp parallel sections num_threads(5)
	{
		#pragma omp section
		{
			osg::ref_ptr<osgViewer::Viewer> viewer = mov.getViewer();

			mov.setVisible(false);
			mov.imageSequence->seek(0.0);
			viewer->frame();

			while (true)
			{
				if (record)
				{
					for (int i = 1; i < mov.numImages; i++)
					{
						//for (int j = 0; j < mov.sequence[i]; j++)
						for (int j = 0; j < N; j++)
						{
							//osg::Timer_t startTick = osg::Timer::instance()->tick();

							mov.imageSequence->seek(((double)i) / ((double)(mov.numImages - 1)));
							viewer->frame();

							//osg::Timer_t endTick = osg::Timer::instance()->tick();
							//double elap_time = osg::Timer::instance()->delta_m(startTick, endTick);

							//printf("%f\n", elap_time);

						}
					}

					if (!loop)
					{
						record = false;
						mov.setVisible(false);
						mov.imageSequence->seek(0.0);
						viewer->frame();
					}

				}
				
				if (!stream)
					break;
			}
		}

		#pragma omp section
		{
			Image img;
			Mat frame, mask, body_mask;
			float left_angle, right_angle;

			while (true)
			{
				if (q.try_dequeue(img))
				{
					unsigned int rowBytes = (double)img.GetReceivedDataSize() / (double)img.GetRows();
					Mat tframe = Mat(img.GetRows(), img.GetCols(), CV_8UC1, img.GetData(), rowBytes);

					frame = tframe.clone();

					threshold(frame, body_mask, body_thresh, 255, THRESH_BINARY_INV);
					threshold(frame, mask, thresh, 255, THRESH_BINARY_INV);

					dilate(body_mask, body_mask, element, Point(-1, -1), 3);
					//dilate(body_mask, body_mask, dilateElement, Point(-1, -1), 3);
					body_mask = Scalar::all(255) - body_mask;

					mask &= body_mask;

					left_angle = 0;
					right_angle = 0;

					if (track)
					{
						morphologyEx(mask, mask, MORPH_OPEN, element, Point(-1, -1), 2);

						//erode(mask, mask, erodeElement, Point(-1, -1), 3);
						//dilate(mask, mask, dilateElement, Point(-1, -1), 3);

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
								drawContours(mask, contours, i, Scalar::all(255), FILLED, 1);
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

					putText(frame, to_string(fps), Point((imageWidth - 50), 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
					putText(frame, to_string(q.size_approx()), Point((imageWidth - 50), 20), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));

					if (record)
					{
						putText(frame, to_string(count), Point(0, 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));

						leftwba.enqueue(left_angle);
						rightwba.enqueue(right_angle);
						rec.enqueue(img);

						count++;
					}

					disp_frame.try_enqueue(frame.clone());
					disp_mask.try_enqueue(mask.clone());

				}

				if (!stream)
					break;
			}
		}

		#pragma omp section
		{
			Image tImage;
			float tleft, tright;

			while (true)
			{
				if (rec.try_dequeue(tImage))
				{
					if (!fout.IsOpen())
					{
						fout.Open();
						fout.InitHeader(imageWidth, imageHeight);
						fout.WriteHeader();
					}

					leftwba.try_dequeue(tleft);
					rightwba.try_dequeue(tright);

					fout.WriteFrame(tImage);
					fout.WriteLog(tImage.GetTimeStamp());
					fout.WriteWBA(tleft, tright);
					fout.nframes++;
				}
				else
				{
					if (!record && fout.IsOpen())
						fout.Close();

					if (!stream)
						break;
				}
			}
		}

		#pragma omp section
		{
			namedWindow("controls", WINDOW_AUTOSIZE);
			createTrackbar("thresh", "controls", &thresh, 255);
			createTrackbar("body thresh", "controls", &body_thresh, 255);

			Mat tframe, tmask;

			while (true)
			{

				if (disp_frame.try_dequeue(tframe))
				{
					line(tframe, Point((imageWidth / 2) - 10, imageHeight / 2), Point((imageWidth / 2) + 10, imageHeight / 2), 255);  //crosshair horizontal
					line(tframe, Point(imageWidth / 2, (imageHeight / 2) - 10), Point(imageWidth / 2, (imageHeight / 2) + 10), 255);  //crosshair vertical

					imshow("image", tframe);
				}

				if (disp_mask.try_dequeue(tmask))
					imshow("mask", tmask);

				waitKey(1);

				if (!stream)
				{
					destroyWindow("image");
					destroyWindow("mask");
					break;
				}
			}
		}

		#pragma omp section
		{
			while (true)
			{

				if (GetAsyncKeyState(VK_F1))
				{
					if (!track_key_state)
						track = !track;

					track_key_state = 1;
				}
				else
					track_key_state = 0;

				if (GetAsyncKeyState(VK_F2))
				{
					if (!record_key_state)
					{
						record = !record;
						count = 0;

						mov.setVisible(record);
					}

					record_key_state = 1;
				}
				else
					record_key_state = 0;


				//if (GetAsyncKeyState(VK_F3))
				//{
				//	if (!laser_key_state)
				//	{
				//		laser = !laser;

				//		if (laser)
				//			SP->WriteData("1", 1);
				//		else
				//			SP->WriteData("0", 1);
				//	}

				//	laser_key_state = 1;

				//}
				//else
				//	laser_key_state = 0;

				if (GetAsyncKeyState(VK_ESCAPE))
				{
					stream = false;
					break;
				}

				if (record && loop)
				{
					if (count == MAXRECFRAMES)
					{
						count = 0;

						record = false;
						mov.setVisible(false);
					}
				}


			}
		}
	}

	if (fout.IsOpen())
		fout.Close();

	//fin.Close();
	wingcam.Stop();

	//if (SP->IsConnected())
	//	SP->WriteData("0", 1);

	return 0;
}
