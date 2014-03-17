#include "FlyCapture2.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/gpu/gpu.hpp>

using namespace FlyCapture2;
using namespace cv;
using namespace cv::gpu;

void PrintBuildInfo();
void PrintCameraInfo(CameraInfo* pCamInfo);
void PrintFormat7Capabilities(Format7Info fmt7Info);
void PrintError(Error error);