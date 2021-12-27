#ifndef FLYWORLD_H
#define FLYWORLD_H

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/ImageSequence>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/CullFace>

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
	double radius;
	double height;
	double sideDistance, centerDistance, depth;
	double camHorLoc, camVertLoc;
	double sideCull, centerCull, sideVPOffset, centerVPOffset, sidecamVertLoc;

	osg::ref_ptr<osgViewer::Viewer> viewer;
	
	const char* trainingImageFileName;
	const char* displayFile;

	osg::ref_ptr<osg::Geode> createShapes();
	void setView();
	void setup();

public:

	osg::ref_ptr<osg::ImageSequence> imageSequence;

	double angle;
	osg::ref_ptr<osg::Geode> sphereNode;
	osg::ref_ptr<osgViewer::Viewer> getViewer();
	void setVisible(bool v);

	FlyWorld(char *trainingImgFile, char *settings, double w, double h, double x, double r, double ht) :
		trainingImageFileName(trainingImgFile), displayFile(settings), viewWidth(w), viewHeight(h), xOffset(x), yOffset(0), radius(r), height(ht), sideDistance(r + 8.0), centerDistance(r + 8.0),
		camHorLoc(0), camVertLoc(-ht / 2), sideCull(0), centerCull(0), depth(0), sideVPOffset(0), centerVPOffset(0), sidecamVertLoc(-ht / 2)

	{
		viewer = new osgViewer::Viewer();
		setup();
	}
};
#endif