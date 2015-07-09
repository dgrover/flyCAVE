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
	hints->setCreateBody(false);
	hints->setCreateBackFace(true);
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
	std::ifstream file(displayFile, std::ios::in);

	if (file.is_open())
	{
		file >> centerDistance;
		file >> centerCull;
		file >> centerVPOffset;
		file >> sideDistance;
		file >> sideCull;
		file >> sideVPOffset;
		file.close();
	}
}

void FlyWorld::setup()
{
	osg::setNotifyLevel(osg::NotifySeverity::ALWAYS);
	
	sphereNode = createShapes();
	
	osg::ref_ptr<osg::Group> root = new osg::Group;
	//root->addChild(createShapes());
	root->addChild(sphereNode.get());

	setVisible(false);
	
	viewer->setSceneData(root);
	setView();

	osg::Matrix reflection = osg::Matrixd::scale(-1, 1, 1);
	osg::Matrix leftRotation = osg::Matrixd::rotate(osg::DegreesToRadians(90.0), osg::Vec3(0, 0, 1));
	osg::Matrix rightRotation = osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0, 0, 1));
	osg::Matrix leftCombined = leftRotation*reflection;
	osg::Matrix rightCombined = rightRotation*reflection;


	osg::ref_ptr<osg::GraphicsContext::Traits> masterTraits = new osg::GraphicsContext::Traits;
	GLenum buffer = masterTraits->doubleBuffer ? GL_BACK : GL_FRONT;
	viewer->getCamera()->setDrawBuffer(buffer);
	viewer->getCamera()->setReadBuffer(buffer);
	viewer->getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	viewer->getCamera()->setCullingMode(osg::CullSettings::ENABLE_ALL_CULLING);
	viewer->getCamera()->setClearColor(osg::Vec4(0, 0, 0, 1)); // black background
	viewer->getCamera()->setProjectionMatrix(osg::Matrixd::identity());
	viewer->getCamera()->setViewMatrix(osg::Matrixd::rotate(osg::DegreesToRadians(-90.0f), osg::Vec3f(0.0f, 0.0f, 0.1f)));
	viewer->setCameraManipulator(NULL);

	osg::ref_ptr<osg::Camera> centerCam = new osg::Camera;
	osg::ref_ptr<osg::GraphicsContext::Traits> centerTraits = new osg::GraphicsContext::Traits;
	centerTraits->x = xOffset + viewWidth;
	centerTraits->y = yOffset;
	centerTraits->width = viewWidth;
	centerTraits->height = viewHeight;
	centerTraits->windowDecoration = false;
	centerTraits->doubleBuffer = true;
	centerTraits->sharedContext = 0;
	osg::ref_ptr<osg::GraphicsContext> centerGC = osg::GraphicsContext::createGraphicsContext(centerTraits.get());
	centerCam->setGraphicsContext(centerGC.get());
	osg::ref_ptr<osg::Viewport> centerVP = new osg::Viewport(centerVPOffset, 0, centerTraits->width, centerTraits->height);
	centerCam->setViewport(centerVP);
	viewer->addSlave(centerCam);
	viewer->getSlave(0)._viewOffset = osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, centerDistance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
	viewer->getSlave(0)._projectionOffset = osg::Matrixd::perspective(40.0, 1280.0 / 4800.0, centerDistance - radius, centerDistance - centerCull);



	osg::ref_ptr<osg::Camera> leftCam = new osg::Camera;
	osg::ref_ptr<osg::GraphicsContext::Traits> leftTraits = new osg::GraphicsContext::Traits;
	leftTraits->x = xOffset;
	leftTraits->y = yOffset;
	leftTraits->width = viewWidth;
	leftTraits->height = viewHeight;
	leftTraits->windowDecoration = false;
	leftTraits->doubleBuffer = true;
	leftTraits->sharedContext = 0;
	osg::ref_ptr<osg::GraphicsContext> leftGC = osg::GraphicsContext::createGraphicsContext(leftTraits.get());
	leftCam->setGraphicsContext(leftGC.get());
	osg::ref_ptr<osg::Viewport> leftVP = new osg::Viewport(-sideVPOffset, 0, leftTraits->width, leftTraits->height);
	leftCam->setViewport(leftVP);
	viewer->addSlave(leftCam);
	viewer->getSlave(1)._viewOffset = leftCombined*osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, sideDistance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
	viewer->getSlave(1)._projectionOffset = osg::Matrixd::perspective(40.0, 1280.0 / 4800.0, sideDistance - radius, sideDistance - sideCull);


	osg::ref_ptr<osg::Camera> rightCam = new osg::Camera;
	osg::ref_ptr<osg::GraphicsContext::Traits> rightTraits = new osg::GraphicsContext::Traits;
	rightTraits->x = xOffset + viewWidth * 2;
	rightTraits->y = yOffset;
	rightTraits->width = viewWidth;
	rightTraits->height = viewHeight;
	rightTraits->windowDecoration = false;
	rightTraits->doubleBuffer = true;
	rightTraits->sharedContext = 0;
	osg::ref_ptr<osg::GraphicsContext> rightGC = osg::GraphicsContext::createGraphicsContext(rightTraits.get());
	rightCam->setGraphicsContext(rightGC.get());
	osg::ref_ptr<osg::Viewport> rightVP = new osg::Viewport(sideVPOffset, 0, rightTraits->width, rightTraits->height);
	rightCam->setViewport(rightVP);
	viewer->addSlave(rightCam);
	viewer->getSlave(2)._viewOffset = rightCombined*osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, sideDistance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
	viewer->getSlave(2)._projectionOffset = osg::Matrixd::perspective(40.0, 1280.0 / 4800.0, sideDistance - radius, sideDistance - sideCull);
}

osg::ref_ptr<osgViewer::Viewer> FlyWorld::getViewer()
{
	return viewer;
}

void FlyWorld::setVisible(bool v)
{
	sphereNode->setNodeMask(v ? 0xffffffff : 0x0);
}