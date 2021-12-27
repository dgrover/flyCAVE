#include "stdafx.h"
#include "fmfwriter.h"

using namespace std;
using namespace FlyCapture2;
using namespace cv;

FmfWriter::FmfWriter()
{
	fp = NULL;
	flog = NULL;
	fwba = NULL;
}

int FmfWriter::Open()
{
	fp = new FILE;
	flog = new FILE;
	fwba = new FILE;

	SYSTEMTIME st;
	GetLocalTime(&st);

	sprintf_s(fname, "D:\\fly2p-%d%02d%02dT%02d%02d%02d.fmf", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(fname);

	sprintf_s(flogname, "D:\\fly2p-log-%d%02d%02dT%02d%02d%02d.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(flogname);

	sprintf_s(fwbaname, "D:\\fly2p-wba-%d%02d%02dT%02d%02d%02d.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(fwbaname);

	sprintf_s(fbgname, "D:\\fly2p-bg-%d%02d%02dT%02d%02d%02d.bmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(fbgname);

	fopen_s(&fp, fname, "wb");

	if (fp == NULL) // Cannot open File
	{
		printf("\nError opening FMF writer. Recording terminated.");
		return -1;
	}

	fopen_s(&flog, flogname, "w");

	if (flog == NULL)
	{
		printf("\nError creating log file. Recording terminated.");
		return -1;
	}

	fopen_s(&fwba, fwbaname, "w");

	if (fwba == NULL)
	{
		printf("\nError creating wing beat angle file. Recording terminated.");
		return -1;
	}

	return 1;

}

int FmfWriter::Close()
{
	//seek to location in file where nframes is stored and replace
	fseek(fp, 20, SEEK_SET);
	fwrite(&nframes, sizeof(unsigned __int64), 1, fp);

	fclose(fp);
	fclose(flog);
	fclose(fwba);

	fp = NULL;
	flog = NULL;
	fwba = NULL;

	return 1;
}

void FmfWriter::InitHeader(unsigned __int32 x, unsigned __int32 y)
{
	//settings for version 1.0 fmf header, take image dimensions as input with number of frames set to zero
	fmfVersion = 1;
	SizeY = y;
	SizeX = x;
	bytesPerChunk = y*x + sizeof(double);
	//bytesPerChunk = y*x;
	nframes = 0;
}


void FmfWriter::WriteHeader()
{
	//write FMF header data
	fwrite(&fmfVersion, sizeof(unsigned __int32), 1, fp);
	fwrite(&SizeY, sizeof(unsigned __int32), 1, fp);
	fwrite(&SizeX, sizeof(unsigned __int32), 1, fp);
	fwrite(&bytesPerChunk, sizeof(unsigned __int64), 1, fp);
	fwrite(&nframes, sizeof(unsigned __int64), 1, fp);
}

void FmfWriter::WriteFrame(Image img)
{
	//double dst = (double) st.seconds;
	double dst = (double)nframes;
	fwrite(&dst, sizeof(double), 1, fp);

	fwrite(img.GetData(), img.GetDataSize(), 1, fp);
}

void FmfWriter::WriteLog(TimeStamp st)
{
	fprintf(flog, "Frame %d - TimeStamp [%d %d %d]\n", nframes, st.cycleSeconds, st.cycleCount, st.cycleOffset);
}

void FmfWriter::WriteWBA(float left, float right, double angle, int distractor_key_state)
{
	fprintf(fwba, "%d %f %f %f %d\n", nframes, left, right, angle, distractor_key_state);
}

void FmfWriter::WriteBG(Mat bg)
{
	imwrite(fbgname, bg);
}

int FmfWriter::IsOpen()
{
	if (fp == NULL) // Cannot open File
		return 0;
	else
		return 1;
}

