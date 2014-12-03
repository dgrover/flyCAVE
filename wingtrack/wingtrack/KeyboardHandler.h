#ifndef KEYBOARDHANDLER_H
#define KEYBOARDHANDLER_H

#include <osgGA/GUIEventHandler>

class KeyboardHandler : public osgGA::GUIEventHandler
{
private:
	int & slaveNum;
	double & rS, & scaleRate, &inc;
	float & transInc, &rotInc;
	osg::Matrixd &cam1StartingView;
	osg::Matrixd &cam1DefaultView;
	osg::Matrixd  &cam1View;

	osg::Matrixd transZforward;
	osg::Matrixd transZbackward;
	osg::Matrixd transXleft; 
	osg::Matrixd transXright;
	osg::Matrixd rotcw; 
	osg::Matrixd rotccw;

public:

	KeyboardHandler( int & s, double& r, double& sr, double& i, float& t, float& ri,
		osg::Matrixd & c1s, osg::Matrixd & c1d, osg::Matrixd & c1v): slaveNum(s), rS(r), scaleRate(sr), inc(i), transInc(t), rotInc(ri),
		cam1StartingView(c1s), cam1DefaultView(c1d), cam1View(c1v)
	{
		transZforward = osg::Matrixd::translate(0.0,0.0,transInc);
		transZbackward = osg::Matrixd::translate(0.0,0.0,transInc*-1);
		transXleft = osg::Matrixd::translate(transInc,0.0,0.0); //trans cam left
		transXright = osg::Matrixd::translate(transInc*-1,0.0,0.0);
		rotcw = osg::Matrixd::rotate(osg::DegreesToRadians(rotInc), osg::Vec3(0,1,0)); //rotate camera clockwise
		rotccw = osg::Matrixd::rotate(osg::DegreesToRadians(rotInc*-1), osg::Vec3(0,1,0));
	}

	virtual bool handle(const osgGA::GUIEventAdapter&,osgGA::GUIActionAdapter&);
};

#endif