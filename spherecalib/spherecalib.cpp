#include "stdafx.h"

int xoffset = 1920;
int yoffset = 0;
double viewWidth = 1280.0;// 1920.0 / 2.0;
double viewHeight = 800.0*2.0;// 1200 * 2;
double radius = 2.0;
double defaultDistance = (radius + 8.0);
double distance = defaultDistance;
double defaultCull = 0.0;
double cull = defaultCull;
double loadedDistance = defaultDistance;
double loadedCull = defaultCull;
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
		case 'W':
			distance = distance - transInc;
			break;

		case 's':
		case 'S':
			distance = distance + transInc;
			break;

		case 'a':
		case'A':
			cull = cull - transInc;
			break;

		case 'd':
		case'D':
			cull = cull + transInc;
			break;

		case ' ':
			distance = loadedDistance;
			cull = loadedCull;
			break;

		case '.':
			distance = defaultDistance;
			cull = defaultCull;
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
	hints->setDetailRatio(10.0f);
	hints->setCreateBody(true);
	hints->setCreateBackFace(false);
	hints->setCreateFrontFace(true);
	hints->setCreateNormals(true);
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
	printf("**********\n\n");
}


void writeInfo()
{
	FILE * file = fopen(displayFile, "w");
	fprintf(file, "%f\n", distance);
	fprintf(file, "%f\n", cull);
	fclose(file);
}

void setStartingViews()
{
	double number;
	std::ifstream file(displayFile, std::ios::in);

	if (file.is_open())
	{
		if (file >> number)
		{
			loadedDistance = number;

			if (file >> number)
			{
				loadedCull = number;
				distance = loadedDistance;
				cull = loadedCull;
			}

			else
			{
				distance = defaultDistance;
				cull = defaultCull;
			}
		}

		else
		{
			distance = defaultDistance;
			cull = defaultCull;
		}

		file.close();
	}

	else
	{
		distance = defaultDistance;
		cull = defaultCull;
	}
}


void setup()
{
	osg::ref_ptr<osg::Group> root = new osg::Group;
	osg::ref_ptr<osg::Geode> geode = createShapes();
	root->addChild(geode);
	viewer.setSceneData(root);
	setStartingViews();
	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
	traits->x = xoffset;
	traits->y = yoffset;
	traits->width = viewWidth;
	traits->height = viewHeight;
	traits->windowDecoration = false;
	traits->doubleBuffer = true;
	traits->sharedContext = 0;

	osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
	viewer.getCamera()->setGraphicsContext(gc.get());
	osg::ref_ptr<osg::Viewport> vp = new osg::Viewport(0, 0, traits->width, traits->height);
	viewer.getCamera()->setViewport(vp);
	GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
	viewer.getCamera()->setDrawBuffer(buffer);
	viewer.getCamera()->setReadBuffer(buffer);
	keyboardHandler* handler = new keyboardHandler();
	viewer.addEventHandler(handler);
	viewer.getCamera()->setClearColor(backgroundColor); // black background
	viewer.setCameraManipulator(NULL);
	viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	viewer.getCamera()->setCullingMode(osg::CullSettings::ENABLE_ALL_CULLING);


	osg::ref_ptr<osg::Camera> cam2 = new osg::Camera;

	osg::ref_ptr<osg::GraphicsContext::Traits> traits2 = new osg::GraphicsContext::Traits;
	traits2->x = xoffset+viewWidth;
	traits2->y = yoffset;
	traits2->width = viewWidth;
	traits2->height = viewHeight;
	traits2->windowDecoration = false;
	traits2->doubleBuffer = true;
	traits2->sharedContext = 0;

	osg::ref_ptr<osg::GraphicsContext> gc2 = osg::GraphicsContext::createGraphicsContext(traits2.get());
	cam2->setGraphicsContext(gc2.get());
	cam2->setViewport(vp);
	viewer.addSlave(cam2);


	viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3d(camHorLoc, distance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), up);
	viewer.getCamera()->setProjectionMatrixAsPerspective(40.0, viewWidth / viewHeight, distance - radius, distance - cull);
	viewer.getSlave(0)._viewOffset = osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-90.0), osg::Vec3(0, 1, 0))*osg::Matrixd::translate(0.0, 0.0, -1 * distance);

	/*
	osg::Matrixd viewMatrix = osg::amera->getViewMatrix(); osg::Matrixd projMatrix = osgcamera->getProjectionMatrix(); GLint viewport[4];
	viewport[0] = osgcamera->getViewport()->x();
	viewport[1] = osgcamera->getViewport()->y();
	viewport[2] = osgcamera->getViewport()->width();
	viewport[3] = osgcamera->getViewport()->height();
	then you might pass viewMatrix.ptr()projMatrix.ptr()viewport to gluUnProject function*/
		
	
}


void run()
{
	while (!viewer.done())
	{
		viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3d(camHorLoc, distance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), up);
		viewer.getCamera()->setProjectionMatrixAsPerspective(40.0, viewWidth / viewHeight, distance - radius, distance - cull);
		viewer.getSlave(0)._viewOffset = osg::Matrixd::translate(0.0, 0.0, distance)*osg::Matrixd::rotate(osg::DegreesToRadians(-110.0), osg::Vec3(0, 1, 0))*osg::Matrixd::translate(0.0, 0.0, -1 * distance);
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