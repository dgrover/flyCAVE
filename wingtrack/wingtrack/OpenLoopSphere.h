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
	bool expansion;
	osg::Matrixd cam1StartingView;
	osg::Matrixd cam1DefaultView;
	osg::Matrixd cam1View;
	osg::ref_ptr<osgViewer::Viewer> viewer;
	double scaleRate;
	double rotationRate;

	const char* imageFileName;
	const char* displayFile;

	osg::ref_ptr<osg::Geode> createShapes();
	void setStartingViews();

public:
	OpenLoopSphere(double w, double h, double x, double y, float c, double d, double s, double r, const char* i, const char* df, bool exp):
		viewWidth(w), viewHeight(h), xOffset(x), yOffset(y),
		cRadius(c), distance(d), scaleRate(s), rotationRate(r), imageFileName(i), displayFile(df), expansion(exp)
	{
		viewer=new osgViewer::Viewer();
		cam1DefaultView=osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0,1,0))*osg::Matrixd::translate(0.0, 0.0, -1*distance);
	}

	osg::Matrixd getView();
	osg::ref_ptr<osgViewer::Viewer> setup();
};
#endif