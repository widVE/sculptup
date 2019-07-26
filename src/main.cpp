//
//  main.cpp
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/10/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

//#pragma warning( disable : 4290 )	//disable warning about exception specification that visual c++ doesn't implement (gcc probably does) - voreen classes cause it

#define WITH_FIONA
#ifdef WIN32
#include "gl/glew.h"
#else
#include "glew/glew.h"
#endif

#include "WorldBuilderScene.h"

#include "FionaUT.h"

#include <Kit3D/glslUtils.h>
#include <Kit3D/glUtils.h>

class FionaScene;
FionaScene* scene = NULL;

extern bool cmp(const std::string& a, const std::string& b);
extern std::string getst(char* argv[], int& i, int argc);

static bool cmpExt(const std::string& fn, const std::string& ext)
{
	std::string extt = fn.substr(fn.rfind('.')+1,100);
	std::cout<<"The extension: "<<extt<<std::endl;
	return cmp(ext.c_str(),extt.c_str());
}

jvec3 curJoy(0,0,0);
jvec3 pos(0,0,0);
quat ori(1,0,0,0);
//int		calibMode = 0;

void draw5Teapot(void) {
	static GLuint phongShader=0;
	static GLuint teapotList =0;

	if( FionaIsFirstOfCycle() )
	{
		pos+=jvec3(0,0,curJoy.z*0.01f);
		ori =exp(YAXIS*curJoy.x*0.01f)*ori;
	}
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTranslate(-pos);
	glRotate(ori);
	glDisable(GL_CULL_FACE);
	glDepthFunc(GL_LESS);
	glLight(vec4(0,0,0,1),GL_LIGHT0,0xFF202020);
	glBindTexture(GL_TEXTURE_2D,0);
	jvec3 pos[5]={jvec3(0,0,-1.5),jvec3(-1.5,0,0),jvec3(1.5,0,0),jvec3(0,-1.5,0), jvec3(0,1.5,0)};
	if( teapotList <=0 )
	{
		teapotList = glGenLists(1);
		glNewList(teapotList,GL_COMPILE);
		glutSolidTeapot(.65f);
		glEndList();
	}
	if( phongShader<=0 )
	{
		glewInit();
		std::string vshader = std::string(commonVShader());
		std::string fshader = coinFShader(PLASTICY,PHONG,false);
		phongShader = loadProgram(vshader,fshader,true);
	}
	glUseProgram(phongShader);
	glEnable(GL_DEPTH_TEST); glMat(0xFFFF8000,0xFFFFFFFF,0xFF404040);
	glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,10);
	for(int i=0;i<3; i++){ glPushMatrix(); glTranslate(pos[i]);
		/*glutSolidTeapot(.65f);*/
		/*glCallList(teapotList);*/
		glSphere(V3ID,.65);
		glPopMatrix();}
	glUseProgram(0);
}

enum APP_TYPE
{
	APP_TEAPOT = 0,
	APP_WORLD_BUILDER=1
} 

type = APP_WORLD_BUILDER;

void wandBtns(int button, int state, int idx)
{
	if(scene)
	{
		scene->buttons(button, state);
	}
}

void keyboard(unsigned int key, int x, int y)
{
	if( type == APP_WORLD_BUILDER) 
	{
		static_cast<WorldBuilderScene*>(scene)->keyboard(key, x, y);
	}
}

void joystick(int w, const jvec3& v)
{
	if(scene) 
	{
		scene->updateJoystick(v);
	}
	curJoy = v;
}

void mouseBtns(int button, int state, int x, int y) {}
void mouseMove(int x, int y) {}


void tracker(int s,const jvec3& p, const quat& q)
{ 
	//equivalent to a "mouse move" function...
	if(s==1 && scene)
	{
		scene->updateWand(p,q); 
	}
}

void render(void)
{
	if(scene!=NULL) 
	{
		scene->render();
	}
	else 
	{
		draw5Teapot();
	}
}

int main(int argc, char *argv[])
{
	glutInit(&argc,argv);
	float measuredIPD=63.5;
	int userID=0;
	bool writeFull=false;
	//for world builder size
	float worldBuilderCubeSize = .5;

	std::string fn;
	for(int i=1; i<argc; i++)
	{
		if(cmp(argv[i], "--ogreScene")) { fn=getst(argv,i,argc); printf("%s\n",fn.c_str());}		
		else if(cmp(argv[i], "--wb")) { type=APP_WORLD_BUILDER; printf("World Builder!\n"); }
		else if(cmp(argv[i], "-size")) {
			worldBuilderCubeSize = atof(argv[++i]); printf("Cube Size set to %f\n", worldBuilderCubeSize);
		}
	}

	switch( type )
	{
		case APP_TEAPOT:
			scene = NULL;
			break;
		case APP_WORLD_BUILDER:
			scene = new WorldBuilderScene();
			scene->navMode = WAND_WORLD;// KEYBOARD;//WAND_MODEL
			//Ross - 2017 - forcing ogre media path to here to fix world builder...
			fionaConf.OgreMediaBasePath = std::string("..\\ext\\Media\\");
			static_cast<WorldBuilderScene*>(scene)->initOgre(fn);
			//set the cube size
			static_cast<WorldBuilderScene*>(scene)->setCubeScale(worldBuilderCubeSize);

			if(fionaConf.appType == FionaConfig::HEADNODE && fionaConf.masterSlave)
			{
				printf("Head node has a secondary machine...\n");
				printf("IP: %s\n", fionaConf.masterSlaveIP.c_str());
				printf("Port: %d\n", fionaConf.masterSlavePort);
			}
			else if(fionaConf.appType == FionaConfig::DEVLAB && fionaConf.masterSlave)
			{
				printf("Dev Lab Head node has a secondary machine...\n");
				printf("IP: %s\n", fionaConf.masterSlaveIP.c_str());
				printf("Port: %d\n", fionaConf.masterSlavePort);
			}
			break;
	}
	
	glutInitDisplayMode(GLUT_RGB|GLUT_DOUBLE|GLUT_DEPTH);
	
	glutCreateWindow	("Window");
	glutDisplayFunc		(render);
	glutJoystickFunc	(joystick);
	glutMouseFunc		(mouseBtns);
	glutMotionFunc		(mouseMove);
	glutWandButtonFunc	(wandBtns);
	glutTrackerFunc		(tracker);
	glutKeyboardFunc	(keyboard);
	
	glutMainLoop();

	//this never actually gets hit with how the code currently works - it goes through FionaUTExit, so don't expect cleanup code placed here to actually clean up anything
	return 0;
}
