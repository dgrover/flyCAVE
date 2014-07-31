#ifndef FMFWRITER_H
#define FMFWRITER_H

//using namespace std;
using namespace FlyCapture2;

class FmfWriter
{
	private:
		FILE *fp;
		FILE *flog;

		char fname[100];
		char flogname[100];

		unsigned __int32 fmfVersion, SizeY, SizeX;
		unsigned __int64 bytesPerChunk;
		//char *buf;

		
	public:
		bool record;
		unsigned __int64 nframes;

		FmfWriter();
		
		int Open();
		int Close();

		void InitHeader(unsigned __int32 x, unsigned __int32 y);
		void WriteHeader();
		void WriteFrame(TimeStamp st, Image img);
		void WriteLog(TimeStamp st);
		
};

#endif