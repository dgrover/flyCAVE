// spherecalib.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

class keyboardHandler : public osgGA::GUIEventHandler
{
	private:
		osg::Matrixd& sphereView;
		float transInc;
		float rotInc;

		osg::Matrixd transZforward;
		osg::Matrixd transZbackward;
		osg::Matrixd transXleft;
		osg::Matrixd transXright;
		osg::Matrixd rotcw;
		osg::Matrixd rotccw;

	public:
		keyboardHandler(osg::Matrixd& sv) : sphereView(sv)
		{
			transInc = 0.1;
			rotInc = 1;

			transZforward = osg::Matrixd::translate(0.0, 0.0, transInc);
			transZbackward = osg::Matrixd::translate(0.0, 0.0, transInc*-1);
			transXleft = osg::Matrixd::translate(transInc, 0.0, 0.0);
			transXright = osg::Matrixd::translate(transInc*-1, 0.0, 0.0);
			rotcw = osg::Matrixd::rotate(osg::DegreesToRadians(rotInc), osg::Vec3(0, 1, 0));
			rotccw = osg::Matrixd::rotate(osg::DegreesToRadians(rotInc*-1), osg::Vec3(0, 1, 0));
		}

		virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
		{
			if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
			{
				switch (ea.getKey())
				{
				case 'a':
					sphereView = sphereView*transXleft;
					break;

				case 'd':
					sphereView = sphereView*transXright;
					break;

				case 'w':
					sphereView = sphereView*transZforward;
					break;

				case 's':
					sphereView = sphereView*transZbackward;
					break;

				case 'q':
					sphereView = sphereView*rotccw;
					break;

				case 'e':
					sphereView = sphereView*rotcw;
					break;

				default:
					return false;
				}

				return true;
			}
			else
				return false;
		}
};

class calibSphere
{
	private:
		osgViewer::Viewer viewer;
		osg::Matrixd sphereView;

		const char* imageFileName = "image.bmp";

		double imageWidth = 912.0;
		double imageHeight = 1140.0;

		double aspectWidth = 1280.0;
		double aspectHeight = 800.0;

		double camHorLoc = 0;
		double camVertLoc = 0;

		float radius = 2.0;
		float projdist = 11.0;

		float fov = 25.0;
		double depth = 0;

		osg::Vec4 backgroundColor = osg::Vec4(0, 0, 0, 1);
		osg::Vec3d up = osg::Vec3d(0, 0, 1); //up vector

	public:
		void setup()
		{
			osg::Geode* geode = new osg::Geode();

			osg::StateSet* stateset = new osg::StateSet();
			stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
			osg::Image* image = new osg::Image();

			image = osgDB::readImageFile(imageFileName);

			if (image)
			{
				osg::Texture2D* texture = new osg::Texture2D(image);
				texture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);

				osg::TexMat* texmat = new osg::TexMat;
				stateset->setTextureAttributeAndModes(0, texture, osg::StateAttribute::ON);
				stateset->setTextureAttributeAndModes(0, texmat, osg::StateAttribute::ON);
				texture->setUnRefImageDataAfterApply(true);
			}

			geode->setStateSet(stateset);
			geode->setCullingActive(false);

			osg::TessellationHints* hints = new osg::TessellationHints;
			hints->setDetailRatio(0.8f);

			hints->setCreateBackFace(true);
			hints->setCreateFrontFace(false);
			hints->setCreateNormals(false);
			hints->setCreateTop(false);
			hints->setCreateBottom(false);

			osg::Sphere* sph = new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), radius);
			osg::ShapeDrawable* sphere = new osg::ShapeDrawable(sph, hints);
			sphere->setUseDisplayList(false);
			geode->addDrawable(sphere);

			osg::Group* root = new osg::Group;
			root->addChild(geode);

			viewer.setSceneData(root);

			osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
			traits->x = 0;
			traits->y = 0;
			traits->width = imageWidth;
			traits->height = imageHeight;
			traits->windowDecoration = false;
			traits->doubleBuffer = true;
			traits->sharedContext = 0;
			traits->screenNum = 1;


			osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());

			osg::ref_ptr<osg::Camera> camera = new osg::Camera;
			camera->setGraphicsContext(gc.get());
			camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
			GLenum buffer = traits->doubleBuffer ? GL_BACK : GL_FRONT;
			camera->setDrawBuffer(buffer);
			camera->setReadBuffer(buffer);
			viewer.addSlave(camera.get(), osg::Matrixd(), osg::Matrixd());

			viewer.getCamera()->setClearColor(backgroundColor); // black background
			viewer.getCamera()->setViewMatrixAsLookAt(osg::Vec3d(camHorLoc, (radius + projdist), camVertLoc), osg::Vec3d(camHorLoc, depth, camVertLoc), up);
			viewer.getCamera()->setProjectionMatrixAsPerspective(fov, aspectWidth / aspectHeight, 0.1, 5);
			
			keyboardHandler* handler = new keyboardHandler(sphereView);
			viewer.addEventHandler(handler);

			viewer.setCameraManipulator(NULL);
		}

		void run()
		{
			while (!viewer.done())
			{
				viewer.getSlave(0)._viewOffset = sphereView;
				viewer.frame();
			}
		}

		void printInfo()
		{
			osg::Quat::value_type  angle = *new osg::Quat::value_type();
			osg::Quat::value_type  rotVecX = *new osg::Quat::value_type();
			osg::Quat::value_type  rotVecY = *new osg::Quat::value_type();
			osg::Quat::value_type  rotVecZ = *new osg::Quat::value_type();

			printf("**********\n");

			printf("Camera X Position: %f\n", viewer.getSlave(0)._viewOffset.getTrans().x());
			printf("Camera Z Position: %f\n", viewer.getSlave(0)._viewOffset.getTrans().z());

			viewer.getSlave(0)._viewOffset.getRotate().getRotate(angle, rotVecX, rotVecY, rotVecZ);

			printf("Camera Rotation: %f\n", osg::RadiansToDegrees(angle));
			printf("Camera Rotation Vector X: %f\n", rotVecX);
			printf("Camera Rotation Vector Y: %f\n", rotVecY);
			printf("Camera Rotation Vector Z: %f\n", rotVecZ);
			printf("\n");

			printf("**********\n\n");
		}
};

int _tmain(int argc, _TCHAR* argv[])
{
	calibSphere cs;
	
	cs.setup();
	cs.run();

	cs.printInfo();
	getchar();

	return 0;
}

