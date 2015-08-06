#include "stdafx.h"

int xOffset = 1920;
int yOffset = 0;

double viewWidth = 912/3;
double viewHeight =  1140*2;

double radius = 1.0;
double defaultDistance = (radius + 8.0);
double centerDistance = defaultDistance;
double sideDistance = defaultDistance;

double defaultCull = 0.0;
double centerCull = defaultCull;
double sideCull = defaultCull;

double loadedCenterDistance = defaultDistance;
double loadedCenterCull = defaultCull;
double loadedSideDistance = defaultDistance;
double loadedSideCull = defaultCull;

double camHorLoc = 0;
double camVertLoc = radius*-1.0;
double transInc = 0.1;
double depth = 0;

int loadedSideVPOffset = 0;
int loadedCenterVPOffset = 0;

int sideVPOffset = 0;
int centerVPOffset = 0;

osg::Vec4 backgroundColor = osg::Vec4(0, 0, 0, 1);
osg::Vec3d up = osg::Vec3d(0, 0, 1); //up vector
const char* imageFileName = "images//numberline.gif";//"vert_stripe.bmp";
const char* displayFile = "..//displaySettings_flycave2.txt";
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
			centerDistance = centerDistance - transInc;
			break;

		case 'W':
			sideDistance = sideDistance - transInc;
			break;


		case 's':
			centerDistance = centerDistance + transInc;
			break;

		case 'S':
			sideDistance = sideDistance + transInc;
			break;

		case 'a':
			centerCull = centerCull - transInc;
			break;

		case'A':
			sideCull = sideCull - transInc;
			break;

		case 'd':
			centerCull = centerCull + transInc;
			break;

		case'D':
			sideCull = sideCull + transInc;
			break;

		case ' ':
			centerDistance = loadedCenterDistance;
			centerCull = loadedCenterCull;
			sideDistance=  loadedSideDistance;
			sideCull = loadedSideCull;
			sideVPOffset = loadedSideVPOffset;
			centerVPOffset = loadedCenterVPOffset;
			break;

		case '.':
			centerDistance = defaultDistance;
			centerCull = defaultCull;
			sideDistance = defaultDistance;
			sideCull = defaultCull;
			sideVPOffset = 0;
			centerVPOffset = 0;
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

		case 'q':
			centerVPOffset++;
			break;

		case 'e':
			centerVPOffset--;
			break;

		case 'Q':
			sideVPOffset++;
			break;

		case 'E':
			sideVPOffset--;
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
	printf("Center Distance: %f\n", centerDistance);
	printf("Center Cull Amount: %f\n", centerCull);
	printf("Center Viewport Offset: %i\n", centerVPOffset);
	printf("Side Distance: %f\n", sideDistance);
	printf("Side Cull Amount: %f\n", sideCull);
	printf("Side Viewport Offset: %i\n", sideVPOffset);
	printf("**********\n\n");
}


void writeInfo()
{
	FILE * file = fopen(displayFile, "w");
	fprintf(file, "%f\n", centerDistance);
	fprintf(file, "%f\n", centerCull);
	fprintf(file, "%i\n", centerVPOffset);
	fprintf(file, "%f\n", sideDistance);
	fprintf(file, "%f\n", sideCull);
	fprintf(file, "%i\n", sideVPOffset);

	fclose(file);
}

void setStartingViews()
{
	std::ifstream file(displayFile, std::ios::in);

	if (file.is_open())
	{
		file >> loadedCenterDistance;
		file >> loadedCenterCull;
		file >> loadedCenterVPOffset;
		file >> loadedSideDistance;
		file >> loadedSideCull;
		file >> loadedSideVPOffset;
		centerDistance = loadedCenterDistance;
		centerCull = loadedCenterCull;
		centerVPOffset = loadedCenterVPOffset;
		sideDistance = loadedSideDistance;
		sideCull = loadedSideCull;
		sideVPOffset = loadedSideVPOffset;
		file.close();
	}

	else
	{
		centerDistance = defaultDistance;
		centerCull = defaultCull;
		centerVPOffset = 0;
		sideDistance = defaultDistance;
		sideCull = defaultCull;
		sideVPOffset = 0;
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
	centerTraits->x = xOffset + viewWidth;
	centerTraits->y = yOffset;
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
	leftTraits->x = xOffset;
	leftTraits->y = yOffset;
	leftTraits->width = viewWidth;
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
	rightTraits->x = xOffset + viewWidth * 2;
	rightTraits->y = yOffset;
	rightTraits->width = viewWidth;
	rightTraits->height = viewHeight;
	rightTraits->windowDecoration = false;
	rightTraits->doubleBuffer = true;
	rightTraits->sharedContext = 0;
	osg::ref_ptr<osg::GraphicsContext> rightGC = osg::GraphicsContext::createGraphicsContext(rightTraits.get());
	rightCam->setGraphicsContext(rightGC.get());
	osg::ref_ptr<osg::Viewport> rightVP = new osg::Viewport(0, 0, rightTraits->width, rightTraits->height);
	rightCam->setViewport(rightVP);
	viewer.addSlave(rightCam);
}

void run()
{
	osg::Matrix reflection = osg::Matrixd::scale(-1, 1, 1);
	osg::Matrix leftRotation = osg::Matrixd::rotate(osg::DegreesToRadians(90.0), osg::Vec3(0, 0, 1));
	osg::Matrix rightRotation = osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0, 0, 1));

	osg::Matrix leftCombined = leftRotation*reflection;
	osg::Matrix rightCombined = rightRotation*reflection;

		
	while (!viewer.done())
	{
		viewer.getSlave(0)._camera->setViewport(centerVPOffset, 0, viewWidth, viewHeight);
		viewer.getSlave(0)._viewOffset = osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, centerDistance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
		viewer.getSlave(0)._projectionOffset = osg::Matrixd::perspective(40.0, 1280.0/4800.0, centerDistance - radius, centerDistance - centerCull);

		viewer.getSlave(1)._camera->setViewport(-sideVPOffset, 0, viewWidth, viewHeight);
		viewer.getSlave(1)._viewOffset = leftCombined*osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, sideDistance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
		viewer.getSlave(1)._projectionOffset = osg::Matrixd::perspective(40.0, 1280.0 / 4800.0, sideDistance - radius, sideDistance - sideCull);

		viewer.getSlave(2)._camera->setViewport(sideVPOffset, 0, viewWidth, viewHeight);
		viewer.getSlave(2)._viewOffset = rightCombined*osg::Matrixd::lookAt(osg::Vec3d(camHorLoc, sideDistance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), osg::Vec3d(0, 0, 1));
		viewer.getSlave(2)._projectionOffset = osg::Matrixd::perspective(40.0, 1280.0 / 4800.0, sideDistance - radius, sideDistance - sideCull);

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