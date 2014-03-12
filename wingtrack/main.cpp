// wingtrack.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "camera.h"

int _tmain(int argc, _TCHAR* argv[])
{
	const Mode k_fmt7Mode = MODE_0;
	const PixelFormat k_fmt7PixFmt = PIXEL_FORMAT_RAW8;

	Error error;

	BusManager busMgr;
	unsigned int numCameras;
	error = busMgr.GetNumOfCameras(&numCameras);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	printf("Number of cameras detected: %u\n", numCameras);

	if (numCameras < 1)
	{
		printf("Insufficient number of cameras... exiting\n");
		return -1;
	}

	PGRGuid guid;
	error = busMgr.GetCameraFromIndex(0, &guid);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	Camera cam;

	// Connect to a camera
	error = cam.Connect(&guid);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Get the camera information
	CameraInfo camInfo;
	error = cam.GetCameraInfo(&camInfo);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	PrintCameraInfo(&camInfo);

	// Query for available Format 7 modes
	Format7Info fmt7Info;
	bool supported;
	fmt7Info.mode = k_fmt7Mode;
	error = cam.GetFormat7Info(&fmt7Info, &supported);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	PrintFormat7Capabilities(fmt7Info);

	if ((k_fmt7PixFmt & fmt7Info.pixelFormatBitField) == 0)
	{
		// Pixel format not supported!
		printf("Pixel format is not supported\n");
		return -1;
	}

	Format7ImageSettings fmt7ImageSettings;
	fmt7ImageSettings.mode = k_fmt7Mode;
	fmt7ImageSettings.offsetX = 0;
	fmt7ImageSettings.offsetY = 0;
	fmt7ImageSettings.width = fmt7Info.maxWidth;
	fmt7ImageSettings.height = fmt7Info.maxHeight;
	fmt7ImageSettings.pixelFormat = k_fmt7PixFmt;

	bool valid;
	Format7PacketInfo fmt7PacketInfo;

	// Validate the settings to make sure that they are valid
	error = cam.ValidateFormat7Settings(
		&fmt7ImageSettings,
		&valid,
		&fmt7PacketInfo);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	if (!valid)
	{
		// Settings are not valid
		printf("Format7 settings are not valid\n");
		return -1;
	}

	// Set the settings to the camera
	error = cam.SetFormat7Configuration(
		&fmt7ImageSettings,
		fmt7PacketInfo.recommendedBytesPerPacket);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Start capturing images
	error = cam.StartCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Retrieve frame rate property
	Property frmRate;
	frmRate.type = FRAME_RATE;
	error = cam.GetProperty(&frmRate);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	printf("Frame rate is %3.2f fps\n", frmRate.absValue);
	
	bool useGPU = true;
	
	Image rawImage;
	
	BackgroundSubtractorMOG mog_cpu;
	MOG_GPU mog_gpu;
	
	Mat fgmask; //fg mask generated by MOG method
	GpuMat d_frame, d_fgmask; 
		
		
	//Ptr<BackgroundSubtractor> pMOG; //MOG Background subtractor
	//pMOG = new BackgroundSubtractorMOG(); //MOG approach

	for (int imageCount = 0; ; imageCount++)
	{
		// Retrieve an image
		error = cam.RetrieveBuffer(&rawImage);
		if (error != PGRERROR_OK)
		{
			PrintError(error);
			continue;
		}

		// Create a converted image
		Image convertedImage;

		// Convert the raw image
		error = rawImage.Convert(PIXEL_FORMAT_MONO8, &convertedImage);
		if (error != PGRERROR_OK)
		{
			PrintError(error);
			return -1;
		}

		// convert to OpenCV Mat
		unsigned int rowBytes = (double)convertedImage.GetReceivedDataSize() / (double)convertedImage.GetRows();
		Mat frame = Mat(convertedImage.GetRows(), convertedImage.GetCols(), CV_8UC1, convertedImage.GetData(), rowBytes);		

		//pMOG->operator()(image, fgMaskMOG);
		
		if (useGPU)
		{
			d_frame.upload(frame);
			mog_gpu(d_frame, d_fgmask, 0.01f);
			d_fgmask.download(fgmask);
		}
        else
            mog_cpu(frame, fgmask, 0.01);
		
		imshow("raw image", frame);
		imshow("FG mask", fgmask);

		char key = waitKey(1);
		
		if (key == 27)
			break;
	}

	// Stop capturing images
	error = cam.StopCapture();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Disconnect the camera
	error = cam.Disconnect();
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	printf("Done! Press Enter to exit...\n");
	getchar();

	return 0;
}

