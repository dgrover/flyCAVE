#include "stdafx.h"
#include "camera.h"

void PrintBuildInfo()
{
	FC2Version fc2Version;
	Utilities::GetLibraryVersion(&fc2Version);
	char version[128];
	sprintf_s(
		version,
		"FlyCapture2 library version: %d.%d.%d.%d\n",
		fc2Version.major, fc2Version.minor, fc2Version.type, fc2Version.build);

	printf("%s", version);

	char timeStamp[512];
	sprintf_s(timeStamp, "Application build date: %s %s\n\n", __DATE__, __TIME__);

	printf("%s", timeStamp);
}

void PrintCameraInfo(CameraInfo* pCamInfo)
{
	printf(
		"\n*** CAMERA INFORMATION ***\n"
		"Serial number - %u\n"
		"Camera model - %s\n"
		"Camera vendor - %s\n"
		"Sensor - %s\n"
		"Resolution - %s\n"
		"Firmware version - %s\n"
		"Firmware build time - %s\n\n",
		pCamInfo->serialNumber,
		pCamInfo->modelName,
		pCamInfo->vendorName,
		pCamInfo->sensorInfo,
		pCamInfo->sensorResolution,
		pCamInfo->firmwareVersion,
		pCamInfo->firmwareBuildTime);
}

void PrintFormat7Capabilities(Format7Info fmt7Info)
{
	printf(
		"Max image pixels: (%u, %u)\n"
		"Image Unit size: (%u, %u)\n"
		"Offset Unit size: (%u, %u)\n"
		"Pixel format bitfield: 0x%08x\n",
		fmt7Info.maxWidth,
		fmt7Info.maxHeight,
		fmt7Info.imageHStepSize,
		fmt7Info.imageVStepSize,
		fmt7Info.offsetHStepSize,
		fmt7Info.offsetVStepSize,
		fmt7Info.pixelFormatBitField);
}

void PrintError(Error error)
{
	error.PrintErrorTrace();
}
