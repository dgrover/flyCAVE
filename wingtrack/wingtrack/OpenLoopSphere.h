#ifndef OPENLOOPSPHERE_H
#define OPENLOOPSPHERE_H


#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include "KeyboardHandler.h"
#include "TextureUpdateCallback.h"

class OpenLoopSphere
{
private:
	double numViews;
	double totalWidth;
	double totalHeight;
	double viewWidth;
	double viewHeight;
	double xOffset;
	double yOffset;
	double camHorLoc;
	double camVertLoc;
	float cRadius;
	double distance;
	int slaveNum;
	float transInc, rotInc;
	double inc;
	bool expansion;
	osg::Matrixd cam1StartingView;
	//osg::Matrixd cam2StartingView;
	//osg::Matrixd cam3StartingView;
	//osg::Matrixd cam4StartingView;

	//osg::Matrixd cam1DefaultView;
	osg::Matrixd cam1DefaultView;
	//osg::Matrixd cam3DefaultView;
	//osg::Matrixd cam4DefaultView;

	osg::Matrixd cam1View;
	//osg::Matrixd cam2View;
	//osg::Matrixd cam3View;
	//osg::Matrixd cam4View;




	osg::ref_ptr<osgViewer::Viewer> viewer;

	osg::Vec3d up; //up vector

	double scaleRate;
	double rS; //rotation speed


	const char* imageFileName;
	const char* displayFile;



	osg::ref_ptr<osg::Geode> createShapes();

	void setStartingViews();

public:

	void printInfo(int cam);
	void writeInfo();
	OpenLoopSphere(double n, double w, double h, double x, double y, double ch, double cv, float c, double d, double s, double r, const char* i, const char* df, bool exp): numViews(n),
		totalWidth(w), totalHeight(h), viewWidth(w/n), viewHeight(h), xOffset(x), yOffset(y),
		camHorLoc(ch), camVertLoc(cv), cRadius(c), distance(d), scaleRate(s), rS(r), imageFileName(i), displayFile(df), expansion(exp)
	{
		slaveNum=0;
		transInc=0.1;
		rotInc=1;
		viewer=new osgViewer::Viewer();

		//cam1DefaultView=osg::Matrixd();
		cam1DefaultView=osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0,1,0))*osg::Matrixd::translate(0.0, 0.0, -1*distance);
		//cam3DefaultView=osg::Matrixd::translate(0.0, 0.0, distance*2)*osg::Matrixd::rotate(osg::DegreesToRadians(180.0), osg::Vec3(0,1,0));
		//cam4DefaultView=osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(90.0), osg::Vec3(0,1,0))*osg::Matrixd::translate(0.0, 0.0, -1*distance);



		inc = 0.1;
		up=osg::Vec3d(0,0,1); //up vector
	}

	osg::Matrixd getView(int v);


	osg::ref_ptr<osgViewer::Viewer> setup();




};





#endif