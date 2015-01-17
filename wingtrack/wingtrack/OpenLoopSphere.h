#ifndef OPENLOOPSPHERE_H
#define OPENLOOPSPHERE_H

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include "TextureUpdateCallback.h"

class OpenLoopSphere
{
private:
	double viewWidth;
	double viewHeight;
	double xOffset;
	double yOffset;
	float cRadius;
	double distance;
	int expansion;
	int clockwise;
	osg::Matrixd cam1StartingView;
	osg::Matrixd cam1DefaultView;
	osg::Matrixd cam1View;
	osg::ref_ptr<osgViewer::Viewer> viewer;
	double scaleOrRotationRate;

	const char* imageFileName;
	const char* displayFile;

	osg::ref_ptr<osg::Geode> createShapes();
	void setStartingViews();

public:

	OpenLoopSphere(double w, double h, double x, float c, double d, int wavelength, int frequency, int clockwise, int exp) :
		viewWidth(w), viewHeight(h), xOffset(x), yOffset(0), cRadius(c), distance(d), expansion(exp), scaleOrRotationRate(frequency*clockwise)
	{
		viewer = new osgViewer::Viewer();
		cam1DefaultView = osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0, 1, 0))*osg::Matrixd::translate(0.0, 0.0, -1 * distance);
	
		if (exp == 0)
		{
			displayFile="rotationDisplaySettings.txt";
			scaleOrRotationRate = ((double)(wavelength*frequency*clockwise)) / 360.0;

			switch (wavelength)
			{
			case 10:
				imageFileName = "10.bmp";
				break;

			case 20:
				imageFileName = "20.bmp";
				break;
			case 30:
				imageFileName = "30.bmp";
				break;
			case 40:
				imageFileName = "40.bmp";
				break;
			case 50:
				imageFileName = "50.bmp";
				break;	
			}
		}

		else
		{
			if (clockwise == 1)
			{
				imageFileName = "blackLine.bmp";
			}

			else
			{
				imageFileName = "whiteLine.bmp";
			}

			displayFile = "NOTHING";// "expansionDisplaySettings.txt";
		}


		imageFileName = "1024.bmp";
	}



	OpenLoopSphere(double w, double h, double x, double y, float c, double d, const char* i, const char* df, int exp, double r):
		viewWidth(w), viewHeight(h), xOffset(x), yOffset(y),
		cRadius(c), distance(d), imageFileName(i), displayFile(df), expansion(exp), scaleOrRotationRate(r)
	{
		viewer=new osgViewer::Viewer();
		cam1DefaultView=osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0,1,0))*osg::Matrixd::translate(0.0, 0.0, -1*distance);
	}

	osg::Matrixd getView();
	osg::ref_ptr<osgViewer::Viewer> setup();
};
#endif