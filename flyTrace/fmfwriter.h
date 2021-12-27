#ifndef FMFWRITER_H
#define FMFWRITER_H

class FmfWriter
{
	private:
		//FILE *fp;
		//FILE *flog;
		//FILE *fwba;

		FILE *fvra;

		//char fname[100];
		//char flogname[100];
		//char fwbaname[100];

		char fvraname[100];

		//unsigned __int32 fmfVersion, SizeY, SizeX;
		//unsigned __int64 bytesPerChunk;
		//char *buf;

		
	public:
		unsigned __int64 nframes;

		FmfWriter();

		int Open();
		int Close();

		//void InitHeader(unsigned __int32 x, unsigned __int32 y);
		//void WriteHeader();
		//void WriteFrame(FlyCapture2::TimeStamp st, FlyCapture2::Image img);
		//void WriteFrame(FlyCapture2::Image img);
		//void WriteLog(FlyCapture2::TimeStamp st);
		//void WriteWBA(float left, float right);
		
		void WriteVRA(float left, float right, double angle);
		int IsOpen();
};

#endif