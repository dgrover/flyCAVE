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

	if (argc == 2)
	{
		f.Open(argv[1]);
		f.ReadHeader();
		f.GetFrameCount();
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

	BackgroundSubtractorMOG mog_cpu;
	mog_cpu.set("nmixtures", 3);
	Mat frame, fgmask;

	for (int imageCount = 0; imageCount != nframes; imageCount++)
	{
		if (argc == 2)
			frame = f.ReadFrame(imageCount);
		else
		 	frame = wingcam.GrabFrame();

		mog_cpu(frame, fgmask, 0.01);

		imshow("raw image", frame);
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
