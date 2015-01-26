#include "stdafx.h"
#include "OpenLoopSphere.h"


void TextureUpdateCallback::operator()(osg::Node*, osg::NodeVisitor* nv)
{
	if (!texMat)
	{
		return;
	}

	if (nv->getFrameStamp())
	{
		double currTime = nv->getFrameStamp()->getSimulationTime();
		//float s = currTime*scaleOrRotationRate;
		float r = currTime*scaleOrRotationRate;

		if (expansion==1)
		{
			r = currTime*scaleOrRotationRate / 6.548;
			//texMat->setMatrix(osg::Matrix::scale(1.0 / (1.0 + r), 1.0, 1.0)*osg::Matrix::translate(((1.0 - (1.0 / (1.0 + r))) - 1.0) / 2.0 + 0.5, 0.0f, 0.0f));
			texMat->setMatrix(osg::Matrix::scale(1.0 / (1.0 + r), 1.0, 1.0)*osg::Matrix::translate(((1.0 - (1.0 / (1.0 + r))) - 1.0) / 2.0 + 0.5, 0.0f, 0.0f));
			//texMat->setMatrix(osg::Matrix::scale(1.0-r,1.0,1.0)*osg::Matrix::translate((r-1.0)/2.0+0.5,0.0f,0.0f));
		}

		else
		{
			texMat->setMatrix(osg::Matrix::translate(r, 0.0f, 0.0f));
		}
	}
}



osg::ref_ptr<osg::Geode> OpenLoopSphere::createShapes()
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
	osg::ref_ptr<osg::TessellationHints> hints = new osg::TessellationHints;
	hints->setDetailRatio(4.0f);
	hints->setCreateBackFace(false);
	hints->setCreateFrontFace(true);
	hints->setCreateNormals(false);
	hints->setCreateTop(false);
	hints->setCreateBottom(false);
	osg::ref_ptr<osg::Sphere> sph = new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), cRadius);
	osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable(sph, hints);
	sphere->setUseDisplayList(false);
	geode->addDrawable(sphere);
	osg::ref_ptr<osg::TexMat> texmat = (osg::ref_ptr<osg::TexMat>)((osg::TexMat*) (stateset->getTextureAttribute(0, osg::StateAttribute::TEXMAT)));
	geode->setUpdateCallback(new TextureUpdateCallback(texmat, scaleOrRotationRate, expansion));
	return geode;
}


osg::Matrixd OpenLoopSphere::getView()
{
	return cam1View;
}


void OpenLoopSphere::setStartingViews()
{
	float number;
	std::vector<float> numbers;
	std::ifstream file(displayFile, std::ios::in);

	if (file.is_open())
	{
		while (file >> number)
		{
			numbers.push_back(number);
		}

		file.close();

		osg::Matrixd m1, m2, m3, m4;
		m1.setTrans(numbers[0], 0.0, numbers[1]);
		m1.setRotate(osg::Quat(osg::DegreesToRadians(numbers[2]), osg::Vec3d(numbers[3], numbers[4], numbers[5])));
		cam1StartingView = m1;
	}

	else
	{
		cam1StartingView = cam1DefaultView;
	}
}



osg::ref_ptr<osgViewer::Viewer> OpenLoopSphere::setup()
{

	osg::ref_ptr<osg::Group> root = new osg::Group;
	root->addChild(createShapes());

	viewer->setSceneData(root);

	setStartingViews();

	cam1View = cam1StartingView;

	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
	traits->x = xOffset;
	traits->y = yOffset;
	traits->width = viewWidth;
	traits->height = viewHeight;
	traits->windowDecoration = false;
	traits->doubleBuffer = true;
	traits->sharedContext = 0;

	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());

	osg::ref_ptr<osg::Camera> camera = new osg::Camera;
	camera->setGraphicsContext(gc.get());
	camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
	GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
	camera->setDrawBuffer(buffer);
	camera->setReadBuffer(buffer);
	viewer->addSlave(camera.get(), osg::Matrixd(), osg::Matrixd());


	viewer->getCamera()->setClearColor(osg::Vec4(0, 0, 0, 1)); // black background
	viewer->getCamera()->setViewMatrixAsLookAt(osg::Vec3d(0.0, distance, 0.0), osg::Vec3d(0.0, 0, 0.0), osg::Vec3d(0, 0, 1));
	viewer->getCamera()->setProjectionMatrixAsPerspective(25.0, 1920.0/1200.0/*((double)viewWidth) / ((double)viewHeight)*/, 0.1, 5.0);
	viewer->setCameraManipulator(NULL);

	return viewer;
}