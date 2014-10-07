#include "stdafx.h"
#include "fmfwriter.h"

using namespace std;
using namespace FlyCapture2;

FmfWriter::FmfWriter()
{
	fp = new FILE;
	flog = new FILE;

	SYSTEMTIME st;
	GetLocalTime(&st);

	sprintf_s(fname, "WingCam-%d%02d%02dT%02d%02d%02d.fmf", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(fname);

	sprintf_s(flogname, "WingLog-%d%02d%02dT%02d%02d%02d.txt", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	remove(flogname);
}

int FmfWriter::Open()
{
	fp = fopen(fname, "wb");

	if(fp == NULL) // Cannot open File
	{
		printf("\nError opening FMF writer. Recording terminated.");
		return -1;	
	}

	flog = fopen(flogname, "w");
		
	if(flog == NULL)
	{
		printf("\nError creating log file. Recording terminated.");
		return -1;
	}

	return 1;

}

int FmfWriter::Close()
{
	//seek to location in file where nframes is stored and replace
	fseek (fp, 20, SEEK_SET );	
	fwrite(&nframes, sizeof(unsigned __int64), 1, fp);

	fclose(fp);
	fclose(flog);

	if (nframes == 0)
	{
		remove(fname);
		remove(flogname);
	}

	return 1;
}

void FmfWriter::InitHeader(unsigned __int32 x, unsigned __int32 y)
{
	//settings for version 1.0 fmf header, take image dimensions as input with number of frames set to zero
	fmfVersion = 1;
	SizeY = y;
	SizeX = x;
	bytesPerChunk = y*x + sizeof(double);
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

void FmfWriter::WriteFrame(TimeStamp st, Image img)
{
	double dst = (double) st.seconds;

	fwrite(&dst, sizeof(double), 1, fp);
	fwrite(img.GetData(), img.GetDataSize(), 1, fp);
}

void FmfWriter::WriteLog(TimeStamp st)
{
	fprintf(flog, "Frame %d - TimeStamp [%d %d]\n", nframes, st.seconds, st.microSeconds);
}
