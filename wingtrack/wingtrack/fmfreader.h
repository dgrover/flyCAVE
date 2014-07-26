#ifndef FMFREADER_H
#define FMFREADER_H

class FmfReader
{
	private:
		FILE *fp;
		unsigned __int32 fmfVersion, SizeY, SizeX;
		unsigned __int64 bytesPerChunk, nframes;
		long maxFramesInFile;
		char *buf;

	public:

		int Open(_TCHAR *fname);
		int Close();

		void ReadHeader();
		int ReadFrame(unsigned long frameIndex);
		cv::Mat ConvertToCvMat();

};

#endif
