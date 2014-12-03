#ifndef TEXTUREUPDATECALLBACK_H
#define TEXTUREUPDATECALLBACK_H

#include <osg/TexMat>


class TextureUpdateCallback : public osg::NodeCallback
{
private:
	osg::ref_ptr<osg::TexMat> texMat;
	double &rS, &scaleRate;
	bool expansion;
public:
	TextureUpdateCallback(osg::ref_ptr<osg::TexMat> tm, double& r, double& s, bool b): texMat(tm), rS(r), scaleRate(s), expansion(b){}

	virtual void operator()(osg::Node* node, osg::NodeVisitor* nv);
};


#endif