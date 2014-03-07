#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>

using namespace cv;
using namespace std;

int thresh = 100;

int main()
{
	Mat frame,foreground,background;
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	namedWindow("Frame", CV_WINDOW_AUTOSIZE);
	namedWindow("Foreground", CV_WINDOW_AUTOSIZE );

	background = imread("bg.png", 0);
	frame = imread("frame.png", 0);

	absdiff(frame, background, foreground);

	threshold(foreground,foreground,thresh,255,THRESH_BINARY);
	medianBlur(foreground,foreground,9);
	erode(foreground,foreground,Mat());
	dilate(foreground,foreground,Mat());

	findContours( foreground, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0) );
	vector<RotatedRect> minEllipse( contours.size() );

	for( int i = 0; i < contours.size(); i++ )
	{
	       	minEllipse[i] = fitEllipse( Mat(contours[i]) );
		drawContours( foreground, contours, i, Scalar(255,255,255), CV_FILLED, 8, hierarchy );		
		ellipse( frame, minEllipse[i], Scalar(0,0,0), 2 );
	}

	imshow("Frame",frame );
	imshow("Foreground",foreground);

	waitKey();
}
