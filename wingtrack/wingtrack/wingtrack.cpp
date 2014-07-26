// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "fmfreader.h"

using namespace cv;
using namespace cv::gpu;

#define USEGPU 1

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2)
	{
		printf("Filename not specified... exiting\n");
		return -1;
	}

	FmfReader fin;

	BackgroundSubtractorMOG mog_cpu;
	MOG_GPU mog_gpu;

	Mat frame, fgmask;
	GpuMat d_frame, d_fgmask; 

	int success;

	success = fin.Open(argv[1]);

	if (!success)
		return -1;

	success = fin.ReadHeader();

	if (!success)
		return -1;
	
	for (int imageCount = 0; ; imageCount++)
	{
		success = fin.ReadFrame(imageCount);
			
		if (success)
		{
			frame = fin.ConvertToCvMat();
			//frame = tFrame.clone();
		}
		else
			break;			
		
		imshow("raw image", frame);

		if (USEGPU)
		{
			d_frame.upload(frame);
			mog_gpu(d_frame, d_fgmask, 0.01f);
			d_fgmask.download(fgmask);
		}
		else
			mog_cpu(frame, fgmask, 0.01);
		
		imshow("FG mask", fgmask);
		
		char key = waitKey(1);
		
		if (key == 27)		//press [ESC] to exit
			break;
	}

	fin.Close();
	
	printf("\nDone! Press Enter to exit...\n");
	getchar();

	return 0;
}

