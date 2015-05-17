#include "stdafx.h"

int xoffset = 1920;
int yoffset = 0;
double viewWidth = 428;// 1280.0 / 3.0;// 1920.0 / 3.0;
double viewWidth2 =  426;
double viewHeight =  800.0*2.0;// 1200 * 2;
double radius = 2.0;
double defaultDistance = (radius + 8.0);
double distance = defaultDistance;
double distance2 = defaultDistance;
double defaultCull = 0.0;
double cull = defaultCull;
double cull2 = defaultCull;
double loadedDistance = defaultDistance;
double loadedCull = defaultCull;
double loadedDistance2 = defaultDistance;
double loadedCull2 = defaultCull;
double camHorLoc = 0;
double camVertLoc = radius*-1.0;
double transInc = 0.1;
double depth = 0;
osg::Vec4 backgroundColor = osg::Vec4(0, 0, 0, 1);
osg::Vec3d up = osg::Vec3d(0, 0, 1); //up vector
const char* imageFileName = "images//numberline.gif";//"vert_stripe.bmp";
const char* displayFile = "displaySettings.txt";
osgViewer::Viewer viewer;

void printInfo();
osg::ref_ptr<osg::Geode> createShapes();
void setStartingViews();
void setup();
void run();


class keyboardHandler : public osgGA::GUIEventHandler
{
public:
	virtual bool handle(const osgGA::GUIEventAdapter&, osgGA::GUIActionAdapter&);
};

bool keyboardHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
	switch (ea.getEventType())
	{
	case(osgGA::GUIEventAdapter::KEYDOWN) :
	{
		switch (ea.getKey())
		{
		case 'w':
			distance = distance - transInc;
			break;

		case 'W':
			distance2 = distance2 - transInc;
			break;


		case 's':
			distance = distance + transInc;
			break;

		case 'S':
			distance2 = distance2 + transInc;
			break;

		case 'a':
			cull = cull - transInc;
			break;

		case'A':
			cull2 = cull2 - transInc;
			break;

		case 'd':
			cull = cull + transInc;
			break;

		case'D':
			cull2 = cull2 + transInc;
			break;

		case ' ':
			distance = loadedDistance;
			cull = loadedCull;
			distance2 = loadedDistance2;
			cull2 = loadedCull2;
			break;

		case '.':
			distance = defaultDistance;
			cull = defaultCull;
			distance2 = defaultDistance;
			cull2 = defaultCull;
			break;

		case '+':
			transInc = transInc*2.0;
			break;

		case '-':
			transInc = transInc / 2.0;
			break;

		case 'p':
			printInfo();
			break;

		default:
			return false;
			break;
		}
		return true;
	}
	default:
		return false;
		break;
	}
}



osg::ref_ptr<osg::Geode> createShapes()
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
	osg::ref_ptr<osg::Sphere> sph = new osg::Sphere(osg::Vec3d(0.0, 0.0, 0.0), radius);
	osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable(sph, hints);
	sphere->setUseDisplayList(false);
	geode->addDrawable(sphere);

	return geode;
}

void printInfo()
{
	printf("**********\n");
	printf("Distance: %f\n", distance);
	printf("Cull Amount: %f\n", cull);
	printf("Distance2: %f\n", distance2);
	printf("Cull2 Amount: %f\n", cull2);
	printf("**********\n\n");
}


void writeInfo()
{
	FILE * file = fopen(displayFile, "w");
	fprintf(file, "%f\n", distance);
	fprintf(file, "%f\n", cull);
	fprintf(file, "%f\n", distance2);
	fprintf(file, "%f\n", cull2);
	fclose(file);
}

void setStartingViews()
{
	std::ifstream file(displayFile, std::ios::in);

	if (file.is_open())
	{
		file >> loadedDistance;
		file >> loadedCull;
		file >> loadedDistance2;
		file >> loadedCull2;
		distance = loadedDistance;
		cull = loadedCull;
		distance2 = loadedDistance2;
		cull2 = loadedCull2;
		file.close();
	}

	else
	{
		distance = defaultDistance;
		cull = defaultCull;
		distance2 = defaultDistance;
		cull2 = defaultCull;
	}
}


void setup()
{
	osg::ref_ptr<osg::Group> root = new osg::Group;
	root->addChild(createShapes());
	viewer.addEventHandler(new keyboardHandler());
	
	viewer.setSceneData(root);
	setStartingViews();

	osg::ref_ptr<osg::GraphicsContext::Traits> masterTraits = new osg::GraphicsContext::Traits;
	GLenum buffer = masterTraits->doubleBuffer ? GL_BACK : GL_FRONT;
	viewer.getCamera()->setDrawBuffer(buffer);
	viewer.getCamera()->setReadBuffer(buffer);
	viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	viewer.getCamera()->setCullingMode(osg::CullSettings::ENABLE_ALL_CULLING);
	viewer.getCamera()->setClearColor(osg::Vec4(0, 0, 0, 1)); // black background
	viewer.getCamera()->setProjectionMatrix(osg::Matrixd::identity());
	viewer.setCameraManipulator(NULL);


	osg::ref_ptr<osg::Camera> centerCam = new osg::Camera;
	osg::ref_ptr<osg::GraphicsContext::Traits> centerTraits = new osg::GraphicsContext::Traits;
	centerTraits->x = xoffset + viewWidth2;
	centerTraits->y = yoffset;
	centerTraits->width = viewWidth;
	centerTraits->height = viewHeight;
	centerTraits->windowDecoration = false;
	centerTraits->doubleBuffer = true;
	centerTraits->sharedContext = 0;
	osg::ref_ptr<osg::GraphicsContext> centerGC = osg::GraphicsContext::createGraphicsContext(centerTraits.get());
	centerCam->setGraphicsContext(centerGC.get());
	osg::ref_ptr<osg::Viewport> centerVP = new osg::Viewport(0, 0, centerTraits->width, centerTraits->height);
	centerCam->setViewport(centerVP);
	viewer.addSlave(centerCam);




	osg::ref_ptr<osg::Camera> leftCam = new osg::Camera;
	osg::ref_ptr<osg::GraphicsContext::Traits> leftTraits = new osg::GraphicsContext::Traits;
	leftTraits->x = xoffset;
	leftTraits->y = yoffset;
	leftTraits->width = viewWidth2;
	leftTraits->height = viewHeight;
	leftTraits->windowDecoration = false;
	leftTraits->doubleBuffer = true;
	leftTraits->sharedContext = 0;
	osg::ref_ptr<osg::GraphicsContext> leftGC = osg::GraphicsContext::createGraphicsContext(leftTraits.get());
	leftCam->setGraphicsContext(leftGC.get());
	osg::ref_ptr<osg::Viewport> leftVP = new osg::Viewport(0, 0, leftTraits->width, leftTraits->height);
	leftCam->setViewport(leftVP);
	viewer.addSlave(leftCam);


	osg::ref_ptr<osg::Camera> rightCam = new osg::Camera;
	osg::ref_ptr<osg::GraphicsContext::Traits> rightTraits = new osg::GraphicsContext::Traits;
	rightTraits->x = xoffset + viewWidth + viewWidth2;
	rightTraits->y = yoffset;
	rightTraits->width = viewWidth2;
	rightTraits->height = viewHeight;
	rightTraits->windowDecoration = false;
	rightTraits->doubleBuffer = true;
	rightTraits->sharedContext = 0;
	osg::ref_ptr<osg::GraphicsContext> rightGC = osg::GraphicsContext::createGraphicsContext(rightTraits.get());
	rightCam->setGraphicsContext(rightGC.get());
	osg::ref_ptr<osg::Viewport> rightVP = new osg::Viewport(0, 0, rightTraits->width, rightTraits->height);
	rightCam->setViewport(rightVP);
	viewer.addSlave(rightCam);

	

	/*
	osg::Matrixd viewMatrix = osg::Camera->getViewMatrix(); osg::Matrixd projMatrix = osgcamera->getProjectionMatrix(); GLint viewport[4];
	viewport[0] = osgcamera->getViewport()->x();
	viewport[1] = osgcamera->getViewport()->y();
	viewport[2] = osgcamera->getViewport()->width();
	viewport[3] = osgcamera->getViewport()->height();
	then you might pass viewMatrix.ptr()projMatrix.ptr()viewport to gluUnProject function*/
		
	
}

void run()
{
	osg::Matrix reflection = osg::Matrixd::scale(-1, 1, 1);
	osg::Matrix leftRotation = osg::Matrixd::rotate(osg::DegreesToRadians(90.0), osg::Vec3(0, 0, 1));
	osg::Matrix rightRotation = osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0, 0, 1));

	osg::Matrix leftCombined = leftRotation*reflection;
	osg::Matrix rightCombined = rightRotation*reflection;

	osg::Vec3d center = osg::Vec3d(camHorLoc, depth, camVertLoc);
		
	while (!viewer.done())
	{
		viewer.getSlave(0)._viewOffset = osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, distance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
		viewer.getSlave(0)._projectionOffset = osg::Matrixd::perspective(40.0, viewWidth / viewHeight, distance - radius, distance - cull);

		viewer.getSlave(1)._viewOffset = leftCombined*osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, distance2, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
		viewer.getSlave(1)._projectionOffset = osg::Matrixd::perspective(40.0, viewWidth2 / viewHeight, distance2 - radius, distance2 - cull2);

		viewer.getSlave(2)._viewOffset = rightCombined*osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, distance2, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
		viewer.getSlave(2)._projectionOffset = osg::Matrixd::perspective(40.0, viewWidth2 / viewHeight, distance2 - radius, distance2 - cull2);

		viewer.frame();
	}

	

	printInfo();
	writeInfo();
}

int _tmain(int argc, _TCHAR* argv[])
{
	setup();
	run();
	return 0;
}