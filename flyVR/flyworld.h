#ifndef FLYWORLD_H
#define FLYWORLD_H

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TexMat>

class TextureUpdateCallback : public osg::NodeCallback
{
private:
	osg::ref_ptr<osg::TexMat> texMat;
	double &angle;

public:
	TextureUpdateCallback(osg::ref_ptr<osg::TexMat> tm, double& r) : texMat(tm), angle(r){}
	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
};

class FlyWorld
{
private:
	double viewWidth;
	double viewHeight;
	double xOffset;
	double yOffset;
	float cRadius;
	double distance;

	osg::Matrixd cam1View;
	osg::ref_ptr<osgViewer::Viewer> viewer;
	
	const char* imageFileName;
	const char* displayFile;

	osg::ref_ptr<osg::Geode> createShapes();
	void setView();

public:

	double angle;

	FlyWorld(char *imgFile, char *settings, double w, double h, double x, float c, double d) :
		imageFileName(imgFile), displayFile(settings), viewWidth(w), viewHeight(h), xOffset(x), yOffset(0), cRadius(c), distance(d)
	{
		viewer = new osgViewer::Viewer();
		cam1View = osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0, 1, 0))*osg::Matrixd::translate(0.0, 0.0, -1 * distance);
		
		//imageFileName = "blackLine.jpg";
		//displayFile = "displaySettings.txt";
		
	}

	osg::Matrixd getView();
	osg::ref_ptr<osgViewer::Viewer> setup();
};
#endif