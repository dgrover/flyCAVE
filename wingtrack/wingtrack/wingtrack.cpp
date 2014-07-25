// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "fmfreader.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/gpu/gpu.hpp>

using namespace cv;
using namespace cv::gpu;

#define USEGPU 1

FILE *FMF_In;
unsigned __int32 fmfVersion, SizeY, SizeX;
unsigned __int64 bytesPerChunk, nframes;
long maxFramesInFile;
char *buf;

BackgroundSubtractorMOG mog_cpu;
MOG_GPU mog_gpu;

Mat frame, fgmask;
GpuMat d_frame, d_fgmask; 

void ReadFMFHeader()
{		
	fread(&fmfVersion, sizeof(unsigned __int32), 1, FMF_In);
	fread(&SizeY, sizeof(unsigned __int32), 1, FMF_In);
	fread(&SizeX, sizeof(unsigned __int32), 1, FMF_In);
	fread(&bytesPerChunk, sizeof(unsigned __int64), 1, FMF_In);
	fread(&nframes, sizeof(unsigned __int64), 1, FMF_In);

	buf = new char[bytesPerChunk];
			
	maxFramesInFile = (unsigned long int)nframes;

	printf(
		"\n*** VIDEO INFORMATION ***\n"
		"FMF Version: %d\n"
		"Height: %d\n"
		"Width: %d\n"
		"Frame Size: %d\n"
		"Number of Frames: %d\n",
		fmfVersion, 
		SizeY, 
		SizeX, 
		bytesPerChunk-sizeof(double), 
		nframes);
}

int ReadFMFFrame(unsigned long frameIndex)
{
	if((long)frameIndex>=0L && (long)frameIndex < maxFramesInFile)
		fseek (FMF_In, frameIndex*bytesPerChunk + 28 , SEEK_SET );
	else
		return -1; // Cannot grab .. illegal frame number

	fread(buf, sizeof(double), 1, FMF_In);
	fread(buf, bytesPerChunk-sizeof(double), 1, FMF_In);
	
	return 1;	
}

void CloseFMF()
{
	fclose(FMF_In);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc != 2)
	{
		printf("Filename not specified... exiting\n");
		return -1;
	}

	FMF_In = fopen(argv[1], "rb");
	
	if(FMF_In == NULL) // Cannot open File
	{
		printf("Cannot open input video file\n");
		return -1;	
	}
		
	ReadFMFHeader();
	
	for (int imageCount = 0; ; imageCount++)
	{
		int success = ReadFMFFrame(imageCount);
			
		if (success)
		{
			Mat tFrame = Mat(SizeY, SizeX, CV_8UC1, buf, (bytesPerChunk-sizeof(double))/SizeY); //byte size of each row of frame
			frame = tFrame.clone();
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

	CloseFMF();
	
	printf("\nDone! Press Enter to exit...\n");
	getchar();

	return 0;
}

