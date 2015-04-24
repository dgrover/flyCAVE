#include "stdafx.h"

int xoffset = 1920;
int yoffset = 0;
int viewWidth = 1280;// 1920;
int viewHeight = 800 * 2;// 1200 * 2;
double radius = 2.0;
double defaultDistance = (radius + 8.0);
double distance = defaultDistance;
double defaultCull = 0.0;
double cull = defaultCull;
double loadedDistance = defaultDistance;
double loadedCull = defaultCull;
double camHorLoc = 0;
double camVertLoc = radius*-1;;
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
	hints->setDetailRatio(0.8f);
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
	float number;
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
	root->addChild(createShapes());
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
	viewer.getCamera()->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
	GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
	viewer.getCamera()->setDrawBuffer(buffer);
	viewer.getCamera()->setReadBuffer(buffer);
	keyboardHandler* handler = new keyboardHandler();
	viewer.addEventHandler(handler);
	viewer.getCamera()->setClearColor(backgroundColor); // black background
	viewer.setCameraManipulator(NULL);
	viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
	viewer.getCamera()->setCullingMode(osg::CullSettings::ENABLE_ALL_CULLING);

}


void run()
{
	while (!viewer.done())
	{
		viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3d(camHorLoc, distance, camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), up);
		viewer.getCamera()->setProjectionMatrixAsPerspective(40.0, 1920.0 / (1200.0*2.0), distance - radius, distance - cull);
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