#ifndef TEXTUREUPDATECALLBACK_H
#define TEXTUREUPDATECALLBACK_H

#include <osg/TexMat>


class TextureUpdateCallback : public osg::NodeCallback
{
private:
	osg::ref_ptr<osg::TexMat> texMat;
	double &scaleOrRotationRate;
	int expansion;
public:
	TextureUpdateCallback(osg::ref_ptr<osg::TexMat> tm, double& r, int b): texMat(tm), scaleOrRotationRate(r), expansion(b){}

	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
};


#endif