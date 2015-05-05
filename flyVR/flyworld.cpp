#include "stdafx.h"
#include "flyworld.h"

void TextureUpdateCallback::operator()(osg::Node*, osg::NodeVisitor* nv)
{
	if (!texMat)
	{
		return;
	}

	if (nv->getFrameStamp())
	{
		float r = CV_PI / 180.0 * angle;
		texMat->setMatrix(osg::Matrix::translate(r, 0.0f, 0.0f));
	}
}

osg::ref_ptr<osg::Geode> FlyWorld::createShapes()
{
	osg::ref_ptr<osg::Geode> geode = new osg::Geode();
	osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet();
	stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	osg::ref_ptr<osg::Image> image = new osg::Image();
	image = osgDB::readImageFile(imageFileName);
	
	if (image)
	{
		osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(image);
		texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
		osg::ref_ptr<osg::TexMat> texmat = new osg::TexMat;
		stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
		stateset->setTextureAttributeAndModes(0, texmat, osg::StateAttribute::ON);
		texture->setUnRefImageDataAfterApply(true);
	}

	geode->setStateSet(stateset);
	geode->setCullingActive(true);
	osg::ref_ptr<osg::CullFace> cull = new osg::CullFace(osg::CullFace::Mode::BACK);
	stateset->setAttributeAndModes(cull, osg::StateAttribute::ON);
	osg::ref_ptr<osg::TessellationHints> hints = new osg::TessellationHints;
	hints->setDetailRatio(2.0f);
	hints->setCreateBackFace(false);
	hints->setCreateFrontFace(true);
	hints->setCreateNormals(false);
	hints->setCreateTop(false);
	hints->setCreateBottom(false);
	osg::ref_ptr<osg::Sphere> sph = new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), radius);
	osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable(sph, hints);
	sphere->setUseDisplayList(false);
	geode->addDrawable(sphere);
	osg::ref_ptr<osg::TexMat> texmat = (osg::ref_ptr<osg::TexMat>)((osg::TexMat*) (stateset->getTextureAttribute(0, osg::StateAttribute::TEXMAT)));
	geode->setUpdateCallback(new TextureUpdateCallback(texmat, angle));
	return geode;
}


void FlyWorld::setView()
{
	double number;
	std::ifstream file(displayFile, std::ios::in);

	if (file.is_open())
	{
		if (file >> number)
		{
			distance = number;

			if (file >> number)
			{
				cull = number;
			}
		}

		file.close();
	}
}

void FlyWorld::setup()
{
	osg::ref_ptr<osg::Group> root = new osg::Group;
	root->addChild(createShapes());
	viewer->setSceneData(root);
	setView();

	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
	traits->x = xOffset;
	traits->y = yOffset;
	traits->width = viewWidth;
	traits->height = viewHeight;
	traits->windowDecoration = false;
	traits->doubleBuffer = true;
	traits->sharedContext = 0;

	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());

	viewer->getCamera()->setGraphicsContext(gc.get());
	osg::ref_ptr<osg::Viewport> vp = new osg::Viewport(0, 0, traits->width, traits->height);
	viewer->getCamera()->setViewport(vp);
	GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
	viewer->getCamera()->setDrawBuffer(buffer);
	viewer->getCamera()->setReadBuffer(buffer);

	viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	viewer->getCamera()->setCullingMode(osg::CullSettings::ENABLE_ALL_CULLING);
	viewer->getCamera()->setClearColor(osg::Vec4(0, 0, 0, 1)); // black background
	viewer->getCamera()->setViewMatrixAsLookAt(osg::Vec3d(camHorLoc, distance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
	viewer->getCamera()->setProjectionMatrixAsPerspective(40.0, viewWidth / viewHeight, distance - radius, distance - cull);
	viewer->setCameraManipulator(NULL);


	osg::ref_ptr<osg::Camera> cam2 = new osg::Camera;

	osg::ref_ptr<osg::GraphicsContext::Traits> traits2 = new osg::GraphicsContext::Traits;
	traits2->x = xOffset + viewWidth;
	traits2->y = yOffset;
	traits2->width = viewWidth;
	traits2->height = viewHeight;
	traits2->windowDecoration = false;
	traits2->doubleBuffer = true;
	traits2->sharedContext = 0;

	osg::ref_ptr<osg::GraphicsContext> gc2 = osg::GraphicsContext::createGraphicsContext(traits2.get());
	cam2->setGraphicsContext(gc2.get());
	cam2->setViewport(vp);
	viewer->addSlave(cam2);
	viewer->getSlave(0)._viewOffset = osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0, 1, 0))*osg::Matrixd::translate(0.0, 0.0, -1 * distance);
}

osg::ref_ptr<osgViewer::Viewer> FlyWorld::getViewer()
{
	return viewer;
}