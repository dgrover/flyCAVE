#ifndef FLYWORLD_H
#define FLYWORLD_H

#include <osgDB/ReadFile>
#include <osgViewer/Viewer>
#include <osg/ShapeDrawable>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/CullFace>
#include <osg/ImageSequence>
#include <osgDB/Registry>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>

class FlyWorld
{
private:
	double viewWidth;
	double viewHeight;
	double xOffset;
	double yOffset;
	double radius;
	double sideDistance, centerDistance, depth;
	double camHorLoc, camVertLoc;
	double sideCull, centerCull, sideVPOffset, centerVPOffset;

	osg::ref_ptr<osgViewer::Viewer> viewer;
	
	const char* imageFileName;
	//const char* sequenceFile;
	const char* displayFile;

	osg::ref_ptr<osg::Geode> createShapes();
	void setView();
	//void setSequence();
	void setup();

public:

	osg::ref_ptr<osg::Geode> sphereNode;
	osg::ref_ptr<osg::ImageSequence> imageSequence;
	//std::vector<int> sequence;
	unsigned int numImages;

	osg::ref_ptr<osgViewer::Viewer> getViewer();
	void setVisible(bool v);
	
	//FlyWorld(char *imgFile, char *sequence, char *settings, double w, double h, double x, double r) :
	//	imageFileName(imgFile), sequenceFile(sequence), displayFile(settings), viewWidth(w), viewHeight(h), xOffset(x), yOffset(0), radius(r), sideDistance(r + 10.0), centerDistance(r+10.0),
	//	camHorLoc(0), camVertLoc(r*-1.0), sideCull(0), centerCull(0), depth(0), sideVPOffset(0), centerVPOffset(0)
	//{
	//	viewer = new osgViewer::Viewer();
	//	setup();
	//}

	FlyWorld(char *imgFile, char *settings, double w, double h, double x, double r) :
		imageFileName(imgFile), displayFile(settings), viewWidth(w), viewHeight(h), xOffset(x), yOffset(0), radius(r), sideDistance(r + 10.0), centerDistance(r + 10.0),
		camHorLoc(0), camVertLoc(r*-1.0), sideCull(0), centerCull(0), depth(0), sideVPOffset(0), centerVPOffset(0)
	{
		viewer = new osgViewer::Viewer();
		setup();
	}
};
#endif