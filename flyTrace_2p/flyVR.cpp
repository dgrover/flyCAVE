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

#define NTRAINING			7		// 7 trials
#define ITI					3000	// 30 sec inter-trial-interval

#define distractor			false	// distractors ON/OFF
#define distractor_1		250
#define distractor_2		1000
#define distractor_3		1750
#define distractor_4		5750

#define BG_NFRAMES			1000

struct {
	bool operator() (cv::Point pt1, cv::Point pt2) { return (pt1.y > pt2.y); }
} mycomp;

bool training = false;

bool stream = false;
bool record = false;
bool comp_bg = true;

bool distractor_delivery = false;

int last = 0, fps = 0;
double tangle = 0.0;

struct save_data
{
	Image rec;
	float leftwba;
	float rightwba;
	double vra;
	int distractor_key_state;
};

ReaderWriterQueue<Image> q(100);
//ReaderWriterQueue<Image> rec(100);
ReaderWriterQueue<Mat> disp_frame(1), disp_mask(1);
ReaderWriterQueue<Mat> disp_bg(1);

ReaderWriterQueue<save_data> data_queue;

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

int _tmain(int argc, _TCHAR* argv[])
{
	FlyWorld vr("images//center_t.bmp", "displaySettings_fly2p.txt", 912 / 3, 1140 * 2, 1920, 2.0, 3.5);
	
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
	
	error = wingcam.SetProperty(FRAME_RATE, 100.0);
	error = wingcam.SetProperty(SHUTTER, 9.926);	//grasshopper
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
		printf("Connecting arduino [OK]\n");

	// turn ON LEDs
	if (SP->IsConnected())
		SP->WriteData("1", 1);

	int thresh = 5;
	//int body_thresh = 150;
	int nerode = 1;
	int ndilate = 3;

	Mat element = getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1));

	//Mat erodeElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));
	//Mat dilateElement = getStructuringElement(MORPH_ELLIPSE, Size(3, 3));

	Point2f center(imageWidth / 2, imageHeight / 2);
	//Point2f bottom(imageWidth / 2, imageHeight);
	Point2f top(imageWidth / 2, 1);

	stream = true;

	Mat frame_bg, frame_mean;
	int count_bg = 0;

	float wing_baseline_total = 0;
	float wing_baseline_avg = 0;
	int count_baseline = 0;

	int duration_count = 0;
	int training_count = 0;

	//Press [F1] to start/stop recording. Press [ESC] to exit
	#pragma omp parallel sections num_threads(5)
	{
		#pragma omp section
		{
			osg::ref_ptr<osgViewer::Viewer> viewer = vr.getViewer();

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

			//int imageCount = 0;

			while (true)
			{
				if (q.try_dequeue(img))
				{
					unsigned int rowBytes = (double)img.GetReceivedDataSize() / (double)img.GetRows();
					Mat tframe = Mat(img.GetRows(), img.GetCols(), CV_8UC1, img.GetData(), rowBytes);

					frame = tframe.clone();

					//threshold(frame, body_mask, body_thresh, 255, THRESH_BINARY_INV);
					//threshold(frame, mask, thresh, 255, THRESH_BINARY_INV);

					if (comp_bg)
					{
						//frame_bg = tframe.clone();
						//comp_bg = false;

						if (count_bg == 0)
						{
							frame_mean = Mat::zeros(imageWidth, imageHeight, CV_32F);
							frame_bg = Mat::zeros(Size(imageWidth, imageHeight), CV_8UC1);
						}

						accumulate(frame, frame_mean);
						count_bg++;

						frame_bg = frame_mean / count_bg;
						frame_bg.convertTo(frame_bg, CV_8UC1);

						if (count_bg == BG_NFRAMES)
							comp_bg = false;

					}

					subtract(frame, frame_bg, mask);
					threshold(mask, mask, thresh, 255, THRESH_BINARY);

					body_mask = Mat::zeros(Size(imageWidth, imageHeight), CV_8UC1);
					ellipse(body_mask, center, Size(75, 35), 90, 0, 360, Scalar(255, 255, 255), FILLED);
					body_mask = Scalar::all(255) - body_mask;

					//dilate(body_mask, body_mask, element, Point(-1, -1), 3);
					//dilate(body_mask, body_mask, dilateElement, Point(-1, -1), 3);
					//body_mask = Scalar::all(255) - body_mask;

					mask &= body_mask;

					left_angle = 0;
					right_angle = 0;

					//morphologyEx(mask, mask, MORPH_OPEN, element, Point(-1, -1), 2);

					erode(mask, mask, element, Point(-1, -1), nerode);
					dilate(mask, mask, element, Point(-1, -1), ndilate);

					// Find contours
					vector<vector<Point>> contours;
					findContours(mask, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

					vector<vector<Point>> hull(contours.size());

					Point2f left, right;

					for (int i = 0; i < contours.size(); i++)
					{
						if (contours[i].size() > 5)
						{
							convexHull(Mat(contours[i]), hull[i], false);

							//drawContours(frame, contours, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());
							drawContours(mask, contours, i, Scalar::all(255), FILLED, 1);
							//drawContours(frame, hull, i, Scalar::all(255), 1, 8, vector<Vec4i>(), 0, Point());

							std::sort(hull[i].begin(), hull[i].end(), mycomp);

							//line(frame, hull[i].front(), center, Scalar(255, 255, 255), 1, LINE_AA);

							if (hull[i].front().x < center.x)
							{
								if (hull[i].front().y > left.y)
									left = hull[i].front();
							}
							else
							{
								if (hull[i].front().y > right.y)
									right = hull[i].front();
							}
						}
					}

					left_angle = angleBetween(left, top, center);

					if (left_angle < 90.0)
						left_angle = 0.0;
					else
						line(frame, left, center, Scalar(255, 255, 255), 1, LINE_AA);

					right_angle = angleBetween(right, top, center);

					if (right_angle < 90.0)
						right_angle = 0.0;
					else
						line(frame, right, center, Scalar(255, 255, 255), 1, LINE_AA);

					putText(frame, to_string(fps), Point((imageWidth - 50), 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
					putText(frame, to_string(q.size_approx()), Point((imageWidth - 50), 20), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
					putText(frame, to_string(count_bg), Point(10, 20), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));

					float wbd = right_angle - left_angle;

					if (record)
					{
						if (duration_count == 0)
						{
							//trigger two-photon start
							if (SP->IsConnected())
								SP->WriteData("4", 1);
						}

						tdata.rec = img;
						tdata.leftwba = left_angle;
						tdata.rightwba = right_angle;
						tdata.vra = tangle;
						tdata.distractor_key_state = distractor_delivery;

						data_queue.enqueue(tdata);

						putText(frame, to_string(training_count), Point(0, 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
						putText(frame, to_string(duration_count), Point(20, 10), FONT_HERSHEY_COMPLEX, 0.4, Scalar(255, 255, 255));
						duration_count++;
					

						if (distractor_delivery)
							distractor_delivery = !distractor_delivery;
					}

					if (training)
					{
						if ((duration_count == distractor_1) || (duration_count == distractor_2) || (duration_count == distractor_3) || (duration_count == distractor_4))
						{
							if (distractor)
							{
								// trigger air-puff
								if (SP->IsConnected())
									SP->WriteData("2", 1);	

								distractor_delivery = true;
							}
						}
						
						if (duration_count == BASE)
							vr.setVisible(true);

						if (duration_count == BASE + CS)
							vr.setVisible(false);

						// trigger IR laser
						if (duration_count == BASE + CS + TRACE_INTERVAL)
							SP->WriteData("3", 1);

						if (duration_count == TOTAL_TRIAL_LENGTH + ITI)
						{
							duration_count = 0;
							training_count++;

							if (training_count == NTRAINING)
							{
								training = false;
								//vr.setVisible(false);
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
					{
						fout.Open();
						fout.InitHeader(imageWidth, imageHeight);
						fout.WriteHeader();
						fout.WriteBG(frame_bg);
					}

					fout.WriteFrame(out_data.rec);
					fout.WriteLog(out_data.rec.GetTimeStamp());
					fout.WriteWBA(out_data.leftwba, out_data.rightwba, out_data.vra, out_data.distractor_key_state);
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
			//createTrackbar("body thresh", "controls", &body_thresh, 255);
			createTrackbar("erode", "controls", &nerode, 5);
			createTrackbar("dilate", "controls", &ndilate, 5);

			Mat tframe, tmask;

			while (true)
			{

				if (disp_frame.try_dequeue(tframe))
				{
					line(tframe, Point((imageWidth / 2) - 10, imageHeight / 2), Point((imageWidth / 2) + 10, imageHeight / 2), 255);  //crosshair horizontal
					line(tframe, Point(imageWidth / 2, (imageHeight / 2) - 10), Point(imageWidth / 2, (imageHeight / 2) + 10), 255);  //crosshair vertical
					ellipse(tframe, center, Size(75, 35), 90, 0, 360, Scalar(255, 255, 255));

					imshow("image", tframe);
				}

				if (disp_mask.try_dequeue(tmask))
					imshow("mask", tmask);

				waitKey(1);

				if (!stream)
				{
					destroyWindow("controls");
					destroyWindow("image");
					destroyWindow("mask");
					break;
				}
			}
		}

		#pragma omp section
		{
			int training_key_state = 0;
			int bg_key_state = 0;

						
			while (true)
			{

				if (GetAsyncKeyState(VK_F1))
				{
					if (!training_key_state)
					{
						training = !training;
						record = training;

						tangle = 0.0;
						
						duration_count = 0;
						training_count = 0;

						if (!training)
							vr.setVisible(false);
					}
					
					training_key_state = 1;
				}
				else
					training_key_state = 0;


				if (GetAsyncKeyState(VK_F4))
				{
					if (!bg_key_state)
					{
						comp_bg = true;
						count_bg = 0;
					}

					bg_key_state = 1;
				}
				else
					bg_key_state = 0;


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

	//turn OFF LEDs
	if (SP->IsConnected())
		SP->WriteData("0", 1);

	return 0;
}
