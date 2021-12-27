// flymovie.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;
using namespace moodycamel;

#define BASE				500		// 5 sec baseline
#define CS					1000	// 10 sec
#define TRACE_INTERVAL		500 	// (delay is negative, trace interval is positive) 0.5 sec before end of CS presentation = -50
#define TOTAL_TRIAL_LENGTH	6000	// 60 sec

#define NTRAINING			14		// 7 trials
#define ITI					3000	// 30 sec inter-trial-interval

#define TEST				2000	// 20 sec
#define GAINCOEFF			0.1

#define distractor			false	// distractors ON/OFF
#define distractor_1		250
#define distractor_2		1000
#define distractor_3		1750
#define distractor_4		5750

struct {
	bool operator() (cv::Point pt1, cv::Point pt2) { return (pt1.y < pt2.y); }
} mycomp; 

bool training = false;
bool test = false;

bool stream = true;
bool record = false;
bool closed_loop = false;
                                       
double tangle = 0.0;

struct save_data
{                     
	float leftwba;
	float rightwba;
	double vra;
};

ReaderWriterQueue<Image> q(200);
ReaderWriterQueue<Mat> disp_frame(1), disp_mask(1);
ReaderWriterQueue<save_data> data_queue;

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

//int sign(float v)
//{
//	return v > 0.0 ? 1 : -1;
//}

int _tmain(int argc, _TCHAR* argv[])
{
	FlyWorld vr("images//small//center_t.bmp", "images//small//center_inv_t.bmp", "images//small//two_t.bmp", "displaySettings_flycave2.txt", 912 / 3, 1140 * 2, 1920, 1.0);
	osg::ref_ptr<osgViewer::Viewer> viewer = vr.getViewer();
	vr.setVisible(false);

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
	
	error = wingcam.SetProperty(FRAME_RATE, 200);
	error = wingcam.SetProperty(SHUTTER, 4.989);
	error = wingcam.SetProperty(GAIN, 0.0);
	//error = wingcam.Start();
	error = wingcam.cam.StartCapture(OnImageGrabbed);

	if (error != PGRERROR_OK)
	{
		error.PrintErrorTrace();
		return -1;
	}

	printf("[OK]\n");

	Serial* SP = new Serial("COM4");    // adjust as needed

	if (SP->IsConnected())
		printf("Connecting air puff / optogenetic laser arduino [OK]\n");

	if (SP->IsConnected())
		SP->WriteData("0", 1);


	Serial* IR = new Serial("COM5");    // adjust as needed

	if (IR->IsConnected())
		printf("Connecting IR LED / laser arduino [OK]\n");

	if (IR->IsConnected())
		IR->WriteData("1", 1);

	int thresh = 157;
	int body_thresh = 120;

	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1));

	//Mat erodeElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	//Mat dilateElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));

	Point2f center(imageWidth / 2, imageHeight / 2);
	Point2f bottom(imageWidth / 2, imageHeight);

	int duration_count = 0;
	int training_count = 0;

	//Press [F1] to start/stop recording. Press [ESC] to exit
	#pragma omp parallel sections num_threads(5)
	{
		#pragma omp section
		{
			//osg::ref_ptr<osgViewer::Viewer> viewer = vr.getViewer();

			while (true)
			{
				if (tangle < 0.0)
					tangle = 359.0;
				
				if (tangle > 359.0)
					tangle = 0.0;
					
				vr.angle = tangle;
				viewer->frame();

				if (!stream)
					break;
			}
		}

		#pragma omp section
		{
			Image img;
			Mat frame, mask, body_mask;
			float left_angle, right_angle;

			save_data tdata;

			int imageCount = 0;

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
							//line(frame, hull[i].back(), center, Scalar(255, 255, 255), 1, LINE_AA);

							//if (hull[i].front().x < center.x)
							//	fly_angle.leftwba = angleBetween(hull[i].front(), hull[i].back(), center);
							//else
							//	fly_angle.rightwba = angleBetween(hull[i].front(), hull[i].back(), center);

							if (hull[i].front().x < center.x)
							{
								float left = angleBetween(hull[i].front(), bottom, center);

								if (left < 90.0)
									left_angle = 0.0;
								else
									left_angle = left;
							}
							else
							{
								float right = angleBetween(hull[i].front(), bottom, center);

								if (right < 90.0)
									right_angle = 0.0;
								else
									right_angle = right;
							}
						}
					}

					putText(frame, to_string(fps), Point((imageWidth - 50), 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
					putText(frame, to_string(q.size_approx()), Point((imageWidth - 50), 20), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));

					if (training || test)
					{
						putText(frame, to_string(training_count), Point(0, 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
						putText(frame, to_string(duration_count), Point(20, 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
					}
					

					float wbd = left_angle - right_angle;

					if (imageCount++ % 2 == 0)
					{
						if (closed_loop)
						{
							if (left_angle == 0 || right_angle == 0)
								tangle += GAINCOEFF;
							else
								tangle -= wbd*GAINCOEFF;
						}
						else
							tangle = 0.0;

						if (record)
						{
							tdata.leftwba = left_angle;
							tdata.rightwba = right_angle;
							tdata.vra = tangle;

							data_queue.enqueue(tdata);
						}

						if (test || training)
							duration_count++;

						if (training)
						{
							if (duration_count == BASE)
								vr.setVisible(true);

							if (duration_count == BASE + CS)
								vr.setVisible(false);

							if ((duration_count == BASE + CS + TRACE_INTERVAL) && (training_count % 2 == 0))
								IR->WriteData("2", 1);

							if (distractor)
							{
								if (duration_count == distractor_1 || duration_count == distractor_2 || duration_count == distractor_3 || duration_count == distractor_4)
									SP->WriteData("1", 1);
							}

							if (duration_count == TOTAL_TRIAL_LENGTH + ITI)
							{
								duration_count = 0;
								training_count++;

								if (training_count % 2 == 0)
									vr.imageSequence->seek(0.0);
								else
									vr.imageSequence->seek(0.5);

								if (training_count == NTRAINING)
								{
									training = false;
									record = false;
								}
							}

						}

						if (test)
						{
							if (duration_count == TEST)
							{
								test = false;

								vr.setVisible(false);
								closed_loop = false;
								record = false;
							}
						}
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
			save_data out_data;

			while (true)
			{
				if (data_queue.try_dequeue(out_data))
				{
					if (!fout.IsOpen())
						fout.Open();

					fout.WriteVRA(out_data.leftwba, out_data.rightwba, out_data.vra);
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
			int f1_key_state = 0;
			int f2_key_state = 0;
						
			while (true)
			{

				if (GetAsyncKeyState(VK_F1))
				{
					if (!f1_key_state)
					{
						training = !training;

						vr.imageSequence->seek(0.0);

						if (!training)
							vr.setVisible(false);

						closed_loop = false;
						record = false;

						tangle = 0.0;
						duration_count = 0;
						training_count = 0;
					}
					
					f1_key_state = 1;
				}
				else
					f1_key_state = 0;

				if (GetAsyncKeyState(VK_F2))
				{
					if (!f2_key_state)
					{
						test = !test;

						record = test;

						vr.imageSequence->seek(1.0);
						tangle = 0.0;
						vr.setVisible(test);
						closed_loop = test;

						duration_count = 0;
					}

					f2_key_state = 1;
				}
				else
					f2_key_state = 0;


				if (GetAsyncKeyState(VK_ESCAPE))
				{
					stream = false;
					break;
				}
			}
		}
	}

	if (fout.IsOpen())
		fout.Close();

	//fin.Close();
	wingcam.Stop();

	if (SP->IsConnected())
		SP->WriteData("0", 1);

	if (IR->IsConnected())
		IR->WriteData("0", 1);

	return 0;
}
