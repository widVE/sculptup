#include "WorldBuilderScene.h"

#define USE_EMG 0

#include <Kit3D/jtrans.h>

#include <Ogre/OgreRoot.h>
#include <Ogre/OgreRenderWindow.h>
#include <Ogre/OgreSceneManager.h>
#include <Ogre/OgreLight.h>
#include <Ogre/OgreEntity.h>
#include <Ogre/OgreHardwarePixelBuffer.h>
#include <Ogre/OgreVector3.h>

#include <Ogre/Sound/OgreOggSound.h>
#include <Ogre/Sound/OgreOggSoundManager.h>
#include <Ogre/OgreMeshSerializer.h>

#include <Ogre/Physics/NxOgrePlaneGeometry.h>
#include <Ogre/Physics/NxOgreRemoteDebugger.h>

#include "OgreDotScene.h"

#include "WorldBuilderSystem.h"
#include "VolumeBuffer.h"
//#include "VolumeRender.h"
#include "VRWBAction.h"

#include "Colors.h"

#include "leap/Leap.h"

const Ogre::Vector3 WorldBuilderScene::CAVE_CENTER = Ogre::Vector3(-5.25529762045281, 1.4478, 5.17094851606241);

Ogre::Matrix4 toOgreWBMat(const tran& p)
{
	return Ogre::Matrix4(p.a00,p.a01,p.a02,p.a03,
						 p.a10,p.a11,p.a12,p.a13,
						 p.a20,p.a21,p.a22,p.a23,
						 p.a30,p.a31,p.a32,p.a33);
}

static float frand() {
    return rand()/float(RAND_MAX);
}

std::string makeTimeString()
{
#ifdef WIN32
	SYSTEMTIME time;
	GetLocalTime(&time);
	char str[128];

	sprintf(str, "%d-%02d-%02d-%02d-%02d-%02d", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
	std::string tstr = str;
#else
		
	time_t tt;
	tt = time(NULL);
	std::string tstr = ctime(&tt);
#endif
	return tstr;
}

void WorldBuilderScene::makeLogDirectory()
{
	m_loggingDirectory = makeTimeString();
	if (!CreateDirectory (m_loggingDirectory.c_str(), NULL))
	{
		printf("couldn't create directory %s\n", m_loggingDirectory.c_str());
		exit(-1);
	}
	printf("creating directory %s\n", m_loggingDirectory.c_str());
}

const float WorldBuilderScene::WII_FIT_ROTATION_SPEED = 0.5f;
const float WorldBuilderScene::EMG_MULTIPLIER = 0.075f;
const float WorldBuilderScene::WAND_OFFSET_DISTANCE = 0.03f;
const float WorldBuilderScene::MINIMUM_FLOW_SIZE = 50.f;

WorldBuilderScene::WorldBuilderScene(void): FionaScene(), sharedContext(NULL), root(NULL), scene(NULL), 
	camera(0), headLight(0), renderTexture(0), vp(0), physicsWorld(0), physicsScene(0), critterRender(0), lastTime(0.f), 
	lastOgreMat(Ogre::Matrix4::IDENTITY), m_worldMode(false), wireframe(false), initialized(false), enableVBO(true), 
	enableSwizzledWalk(false), isolevel(0.5f), gridData(0), m_wbSys(0), /*m_wbRenderer(0),*/ m_cubeScale(0.5f), m_flowMultiplier(200.f), m_cubeWireFrame(true),
    m_wiiFitRotation(0.f), m_currColor(1.0, 0.0, 0.0), m_addMode(0), m_tsr(0), m_currentTool(0), m_raymarchRender(false), m_wandlogfile(0), m_addCube(0.1f, 0.1f, 0.1f),
	m_beamRadius(0.05f), m_beamLength(0.2f), m_replayFile(0), mSoundManager(0), enableDisplayList(true), m_DisplayList(0), gridDataVAO(0)
{
	float cube = 64.f;
	int data = 128;
	cubeSize=jvec3(cube,cube,cube);
	cubeStep=jvec3(2.0f / cubeSize.x, 2.0f / cubeSize.y, 2.0f / cubeSize.z);
	dataSize=vec3i(data,data,data);
	//for data logging
	//make sure we are the head node
	/*if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED))
	{
		makeLogDirectory();
		std::string name = m_loggingDirectory;
		name+="/wand-log.txt";

		m_wandlogfile = fopen(name.c_str(), "w");
		fprintf(m_wandlogfile, "time, wandX, wandY, wandZ, wandDirX, wandDirY, wandDirZ, joyX, joyY, joyZ, buttons\n"); 

		if (0)
		{
			m_replayFile = fopen("wand-log.txt", "r");
		}
	}*/
}

WorldBuilderScene::~WorldBuilderScene()
{
	NxOgre::World::destroyWorld();

    if(gridData)
    {
        delete [] gridData;
    }

	if(m_wandlogfile != 0)
	{
		fclose(m_wandlogfile);
	}

	if(m_replayFile != 0)
	{
		fclose(m_replayFile);
	}

	if (m_DisplayList)
		glDeleteLists(m_DisplayList, 1);

	if (gridDataVAO)
	{
		glDeleteVertexArrays(1, &gridDataVAO);
		glDeleteBuffers(1, &gridDataBuffId);
	}
}

void WorldBuilderScene::addResPath(const std::string& path)
{
	Ogre::ResourceGroupManager& rman=Ogre::ResourceGroupManager::getSingleton();
	rman.addResourceLocation(path, "FileSystem");
}

void WorldBuilderScene::addResourcePaths(void)
{

#ifdef __APPLE__
	addResPath(fionaConf.OgreMediaBasePath+std::string("/Models"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("/Materials/scripts"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("/Materials/textures"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("/Materials/programs"));
#elif defined WIN32
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Models"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Materials\\scripts"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Materials\\textures"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Materials\\programs"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\NxOgre"));
	addResPath(fionaConf.OgreMediaBasePath+std::string("\\Box"));
	//addResPath(fionaConf.OgreMediaBasePath+std::string("\\levels\\2013-01-06-20-32-47\\mesh"));
	if(sceneName.length() > 0)
	{
		/*size_t s = sceneName.find_last_of('\\');
		if(s != -1)
		{
			std::string dir = sceneName.substr(0, s-1);*/
			//get directory from scene name and add sub-dirs
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\levels\\") + sceneName + std::string("\\"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\levels\\") + sceneName + std::string("\\mesh"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\levels\\") + sceneName + std::string("\\material"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\levels\\") + sceneName + std::string("\\bitmap"));
		//}
	}
#endif
}

void WorldBuilderScene::buttons(int button, int state)
{
	bool bWorld = m_worldMode;

	FionaScene::buttons(button, state);

	if(bWorld != m_worldMode)
	{
		if(m_worldMode)
		{
			m_actions.SetCurrentSet("world_mode");
		}
		else
		{
			m_actions.SetCurrentSet("sculpt_mode");
		}
	}
}


void WorldBuilderScene::changeAddMode(void)
{
	m_addMode++;
	m_addMode = m_addMode % WorldBuilderScene::NUM_ADD_MODES;
	// Play a sound to tell the user which tool they've switched to.
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
		switch (m_addMode) {
		case BLOB:
			playSound("spheres");
			break;
		case CUBE:
			playSound("cubes");
			break;
		}
	}
}

void WorldBuilderScene::changeTool(void)
{
	m_currentTool++;
	m_currentTool = m_currentTool % WorldBuilderScene::NUM_TOOLS;
	// Play sounds!
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
		switch (m_currentTool) {
		case ERASER:
			playSound("erase");
			break;
		case PAINT:
			playSound("paint");
			break;
		case SCULPTER:
			// Play the appropriate sound based on which sculpt mode we're in
			switch (m_addMode) {
			case BLOB:
				playSound("spheres");
				break;
			case CUBE:
				playSound("cubes");
				break;
			}
			break;
		}
	}
}

void WorldBuilderScene::changeTSR(void)
{
	m_tsr++;
	m_tsr = m_tsr % WorldBuilderScene::NUM_TRANSFORM_MODES;
	// Play a sound to tell the user which mode they've switched to.
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
		switch (m_tsr) {
		case ROTATE:
			playSound("rotate");
			break;
		case TRANSLATE:
			playSound("position");
			break;
		case SCALE:
			playSound("scale");
			break;
		}
	}
}

void WorldBuilderScene::changeModes(bool bWorldMode)
{
	if(!m_worldMode)
	{
		//make a mesh out of our sculpted work (read results back)
		//m_wbSys->readResults();
 		//makeMesh(m_wbSys->getStateBuffer()->getData(), m_wbSys->getStateBuffer()->getRedData(), m_wbSys->getStateBuffer()->getGreenData(), m_wbSys->getStateBuffer()->getBlueData());
	}

	//todo - perform any other necessary actions when changing modes here...
	//also - how to detect the changing of modes - kinect? tracker height change?
	m_worldMode = bWorldMode;
	if(m_worldMode)
	{
		//take our current sculpted structure and tesselate it..
		navMode = WAND_WORLD;
		glUseProgram(NULL);
		glDisable(GL_TEXTURE_3D);
	}
	else
	{
		clearSelection();

		m_wbSys->reset();

		glShadeModel (GL_SMOOTH);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);

		//Form multi-face view
		glDisable(GL_CULL_FACE);

		glDepthMask(true);
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glClearDepth(1.0);
		navMode = WAND_MODEL;
		camPos.set(0.f, 0.f, 0.f);
		camOri.set(1.f, 0.f, 0.f, 0.f);
	}
	// Play a sound to let the user know which mode we're in now
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
		if (m_worldMode) {
			playSound("world_mode");
		} else {
			playSound("sculpt_mode");
		}
	}
}

void WorldBuilderScene::clearSelection(void)
{
	std::vector<Ogre::MovableObject*>::iterator iter = m_currentSelection.begin();
	while(iter != m_currentSelection.end())
	{
		(*iter)->getParentSceneNode()->showBoundingBox(false);
		iter++;
	}

	m_currentSelection.clear();
}

void WorldBuilderScene::exportPlyScene(void)
{
	VRExportPly exportPly;
	exportPly.SetScenePtr(this);
	exportPly.ButtonUp();
}

void WorldBuilderScene::onExit(void)
{
	printf("Exporting user's scene to ply files...\n");
	//exportPlyScene();
	printf("Exporting user's OGRE .scene file...\n");
	//VRSave save;
	//save.SetScenePtr(this);
	//save.ButtonUp();
}

void WorldBuilderScene::initOgre(std::string scene)
{
	//called from main.cpp

	sceneName = scene;
#ifdef WIN32
#ifdef _DEBUG
	root = new Ogre::Root("Plugins_d.cfg");
#else
	root = new Ogre::Root("Plugins.cfg");
#endif
	std::string workDir="";
#endif
		
	Ogre::RenderSystem* _rs=root->getRenderSystemByName("OpenGL Rendering Subsystem");
	root->setRenderSystem(_rs);
	_rs->setConfigOption(Ogre::String("RTT Preferred Mode"), Ogre::String("FBO"));
	root->initialise(false);
	
	Ogre::ResourceGroupManager& rman=Ogre::ResourceGroupManager::getSingleton();

	addResPath(workDir);
	addResPath(".");
	addResPath(fionaConf.OgreMediaBasePath);

	// User resource path
	addResourcePaths();
}

void WorldBuilderScene::initWin(WIN nwin, CTX ctx, int gwin, int w, int h)
{
	Ogre::NameValuePairList misc;

	HWND hWnd = (HWND)nwin;
	HGLRC hRC = (HGLRC)ctx;
	misc["externalWindowHandle"] = Ogre::StringConverter::toString((int)hWnd);
	misc["externalGLContext"] = Ogre::StringConverter::toString((unsigned long)hRC);
	misc["externalGLControl"] = "true";
	Ogre::RenderWindow* win = root->createRenderWindow("Ogre"
			+Ogre::StringConverter::toString(wins.size()),1920,1920,false,&misc);

	bool firstWindow = scene ==NULL;
	// Creating scene manager and setup other managers
	if( firstWindow ) // Meaning it is the first window ever.
	{
		scene = root->createSceneManager(Ogre::ST_GENERIC);
		scene->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));
		//Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
		Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(10);
			
		camera = scene->createCamera("OgreGLUTDefaultCamera");
		camera->setNearClipDistance(0.01f);
		camera->setFarClipDistance(10000000);
		camera->lookAt(Ogre::Vector3(0,0,-1));
		camera->setPosition(Ogre::Vector3(0,0.0,0));
		//camera->setPosition(Ogre::Vector3(0,10.0,0));
			
		/*headLight=scene->createLight("OgreGLUTDefaultHeadLight");
		headLight->setType(Ogre::Light::LT_POINT);
		headLight->setDiffuseColour(1, 1, 1);
		headLight->setPosition(Ogre::Vector3(1,2,2));*/
	}
	vp = win->addViewport(camera);
	vp->setDimensions(0.0f, 0.0f, 1.0f, 1.0f);
		
	// Add to management system
	wins.push_back(WorldBuilderSceneWinInfo(win,nwin,gwin,ctx,vp));
	if(firstWindow) 
	{
		setupScene(scene);
	}
}

void WorldBuilderScene::keyboard(unsigned int key, int x, int y)
{
	printf("pressed a key %c\n", key);
	if (key == 'R')
	{
		m_replayDumpPLY= !m_replayDumpPLY;
		printf("dumplpy: %d\n", m_replayDumpPLY);
	}
}

void WorldBuilderScene::handleLogging(void)
{
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED))
	{
		if( FionaIsFirstOfCycle() )
		{
			if(m_replayFile)
			{
				const int bufsize = 1024;
				char buf[bufsize];
				fgets(buf, bufsize, m_replayFile);
				float time,wx,wy,wz,wa,wb,wc,wd,cx,cy,cz,ca,cb,cc,cd,jx,jy,jz,r,g,b;
				int button;
				//time,wx,wy,wz,wa,wb,wc,wd,cx,cy,cz,ca,cb,cc,cd,jx,jy,jz,but,r,g,b
				if (sscanf(buf,"%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%d,%f,%f,%f\n", 
					&time, 
					&wx,&wy,&wz, 
					&wa,&wb,&wc, &wd,
					&cx,&cy,&cz, 
					&ca,&cb,&cc, &cd,
					&jx, &jy, &jz, 
					&button,
					&r, &g, &b
					) > 0)
				{
					//lets replace everything
					static float log_lastTime = 0.f;

					fionaConf.physicsTime = time;
					fionaConf.physicsStep = (time-log_lastTime);
					printf("%f : %f\n", time, fionaConf.physicsStep);
					log_lastTime = time;
			
					FionaScene::updateWand(jvec3(wx,wy,wz), quat(wd,wa,wb,wc));
					FionaScene::camPos = jvec3(cx,cy,cz);
					FionaScene::camOri = quat(cd,ca,cb,cc);

					FionaScene::updateJoystick(jvec3(jx,jy,jz));
					fionaConf.currentButtons = button;

					m_currColor = jvec3(r,g,b);
					
					static int lastButton = 0;
					
					if(button != lastButton)
					{
						if (button)
							buttons(5,1);
						else
							buttons(5,0);
					}

					lastButton = button;
					
					/*
					FionaScene.wandPos.x = x;


					fionaConf.currentJoystick.y = y;
					fionaConf.currentJoystick.z = z;

					fionaConf.currentJoystick.x = x;
					fionaConf.currentJoystick.y = y;
					fionaConf.currentJoystick.z = z;

			
						*/
					//printf("time = %f\n", time);

					//make the mesh
					if (m_replayDumpPLY)
					//also check for button?
					if (button)
					{
						//m_wbSys->readResults();
					//m_wbSys->add(0.f, 0.f, 0.f, 0.f, m_cubeScale, m_currColor.x, m_currColor.y, m_currColor.z, true);
 						//makeMesh(m_wbSys->getStateBuffer()->getData(), m_wbSys->getStateBuffer()->getRedData(), m_wbSys->getStateBuffer()->getGreenData(), m_wbSys->getStateBuffer()->getBlueData());
				
					}
				}
		
			}

			if (m_wandlogfile)
			{
				jvec3 wpos = wandPos;
				quat wquat = wandOri;
				jvec3 campos = camPos;
				quat camori = camOri;
				//time,wx,wy,wz,wa,wb,wc,wd,cx,cy,cz,ca,cb,cc,cd,jx,jy,jz,but,r,g,b
				//time
				fprintf(m_wandlogfile, "%f,", fionaConf.physicsTime);
				//wand
				fprintf(m_wandlogfile, "%f,%f,%f,", wpos.x, wpos.y, wpos.z);
				fprintf(m_wandlogfile, "%f,%f,%f,%f,", wquat.x, wquat.y, wquat.z, wquat.w);
				//cam
				fprintf(m_wandlogfile, "%f,%f,%f,", campos.x, campos.y, campos.z);
				fprintf(m_wandlogfile, "%f,%f,%f,%f,", camori.x, camori.y, camori.z, camori.w);
				//joystick
				fprintf(m_wandlogfile, "%f,%f,%f,", fionaConf.currentJoystick.x, fionaConf.currentJoystick.y, fionaConf.currentJoystick.z);
				fprintf(m_wandlogfile, "%d,", fionaConf.currentButtons);
				//color
				fprintf(m_wandlogfile, "%f,%f,%f,", m_currColor.x, m_currColor.y, m_currColor.z);
				//that's all folks
				fprintf(m_wandlogfile, "\n");

				/*
				//trackerPos = getNormalizedWandPos(vWandDir.x, vWandDir.y, vWandDir.z);
				//write out the info to the file
				//fprintf(m_wandlogfile, "time, wandX, wandY, wandZ, wandDirX, wandDirY, wandDirZ, joyX, joyY, joyZ, buttons\n"); 
				fprintf(m_wandlogfile, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %u\n", fionaConf.physicsTime, 
					pos.x, pos.y, pos.z, 
					vWandDir.x, vWandDir.y, vWandDir.z, 
					fionaConf.currentJoystick.x, fionaConf.currentJoystick.y, fionaConf.currentJoystick.z,
					fionaConf.currentButtons
					);
					*/
				//printf("ptime = %f\n", fionaConf.physicsTime);
			}
		}
	}
}

void WorldBuilderScene::playSound(const char *name)
{
	/*if(mSoundManager)
	{
		OgreOggSound::OgreOggISound *pSound = mSoundManager->getSound(name);
		{
			pSound->play();
		}
	}*/
}

void WorldBuilderScene::render(void)
{
	FionaScene::render();

	//update the current speech color...
	static int rCount = 0;

	WIN win = FionaUTGetNativeWindow();
	if( !isInited(win) )
	{
		CTX ctx = FionaUTGetContext(); 
		initWin(win,ctx,glutGetWindow(),
				glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
		
		glewInit();
		initMarchingCube();
	}

	if( _FionaUTIsInFBO() )
	{
		int i = findWin(glutGetWindow());
	}
	else
	{
		//this is screwing things up.. ogre 1.8..
		int wid = glutGet(GLUT_WINDOW_WIDTH);
		int hei = glutGet(GLUT_WINDOW_HEIGHT);
		//printf("%d, %d\n", wid, hei);
		resize(glutGetWindow(),wid,hei);
	}

	jvec3 prevColor = m_currColor;

	if(!m_replayFile)
	{
		m_currColor[0] = (float)color_data[(unsigned int)fionaConf.speechVal].rgb.r/255.f;
		m_currColor[1] = (float)color_data[(unsigned int)fionaConf.speechVal].rgb.g/255.f;
		m_currColor[2] = (float)color_data[(unsigned int)fionaConf.speechVal].rgb.b/255.f;
	}
	
	if( FionaIsFirstOfCycle() )
	{
		if(physicsWorld)
		{
			physicsWorld->advance(fionaConf.physicsStep);
		}
	}

	//handleLogging();

	/*if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED))
	{
		if((prevColor - m_currColor).len() != 0.f)
		{
			mSoundManager->getSound("click")->play();
		}
	}*/

	tran m, p;
	glGetFloatv(GL_MODELVIEW_MATRIX, m.p);
	glGetFloatv(GL_PROJECTION_MATRIX, p.p);
	lastOgreMat = toOgreWBMat(m);
	camera->setCustomViewMatrix(TRUE, lastOgreMat);
	camera->setCustomProjectionMatrix(TRUE, toOgreWBMat(p));

	//we push and pop here because that way we retain the original model view matrix that fiona calculates
	//before the ogre rendering happens..
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	root->renderOneFrame();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	//otherwise the model view ends up being the matrix of that last object rendered in the ogre scene graph
	//so tha scultping ends up being centered around that object which we don't want..


	//render sculpting interface if we aren't in world mode..
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_TEXTURE);
	glPushMatrix();
	glLoadIdentity(); //Texture addressing should start out as direct.
	glMatrixMode(GL_MODELVIEW);

	static Ogre::Pass* clearPass = NULL;
	if (!clearPass)
	{
		Ogre::MaterialPtr clearMat = Ogre::MaterialManager::getSingleton().getByName("BaseWhite");
		clearPass = clearMat->getTechnique(0)->getPass(0);
	}
	//Set a clear pass to give the renderer a clear renderstate
	scene->_setPass(clearPass, true, false);

	if(!m_worldMode)
	{
		/*GLboolean depthTestEnabled=glIsEnabled(GL_DEPTH_TEST);
		glDisable(GL_DEPTH_TEST);
		GLboolean stencilTestEnabled = glIsEnabled(GL_STENCIL_TEST);
		glDisable(GL_STENCIL_TEST);*/
		// save attrib

		glPushAttrib(GL_ALL_ATTRIB_BITS);
		//if we're not in world mode, render scultping
		renderMarchingCubes();

		/*if(m_raymarchRender) {
			m_wbRenderer->setVolume(m_wbSys->getStateBuffer());
			m_wbRenderer->render();
		}*/

		glPopAttrib();

		/*if (depthTestEnabled)
		{
			glEnable(GL_DEPTH_TEST);
		}
		if (stencilTestEnabled)
		{
			glEnable(GL_STENCIL_TEST);
		}*/
	}
	else
	{
		//draw the "wand"
		bool bDrawWand = true;
		if(VRAction *pAction = m_actions.GetCurrentSet()->GetCurrentAction())
		{
			if(pAction->GetType() == VRAction::WAND)
			{
				bDrawWand = static_cast<VRWandAction*>(pAction)->IsCaptureWand();
			}
		}

		static const float fLineLength = 25.f;
		if(bDrawWand)
		{
			jvec3 vPos;
			getWandWorldSpace(vPos, true);
			//this correctly orients the direction of fire..
			jvec3 vWandDir;
			getWandDirWorldSpace(vWandDir, true);
			//if in world mode, let's draw a wand "beam"
			glPushMatrix();
			glLineWidth(4.f);
			glDisable(GL_LIGHTING);
			if(m_tsr == WorldBuilderScene::TRANSLATE)
			{
				glColor3f(1.f, 0.f, 0.f);
			}
			else if(m_tsr == WorldBuilderScene::ROTATE)
			{
				glColor3f(0.f, 1.f, 0.f);
			}
			else if(m_tsr == WorldBuilderScene::SCALE)
			{
				glColor3f(0.f, 0.f, 1.f);
			}
			glBegin(GL_LINES);
			//these two lines are for keyboard testing..
			//glVertex3f(0.f, 0.f, 0.f);
			//glVertex3f(vWandDir.x*fLineLength, vWandDir.y*fLineLength, vWandDir.z*fLineLength);
			//these would be for the cave..
			glVertex3f(vPos.x, vPos.y, vPos.z);
			glVertex3f(vPos.x + vWandDir.x*fLineLength, vPos.y + vWandDir.y * fLineLength, vPos.z + vWandDir.z * fLineLength);
			glEnd();
			//glTranslatef(vPos.x + vWandDir.x*fLineLength, vPos.y + vWandDir.y * fLineLength, vPos.z + vWandDir.z * fLineLength);
			//glutWireCube(1.0);
			glLineWidth(1.f);
			glColor3f(1.f, 1.f, 1.f);
			glEnable(GL_LIGHTING);
			glPopMatrix();
		}

		bool bDrawAxis = true;
		if(bDrawAxis)
		{
			glPushMatrix();
			glTranslatef(0.f, -CAVE_CENTER.y + 0.05f, 0.f);
			glLineWidth(4.f);
			glDisable(GL_LIGHTING);
			glColor3f(1.f, 0.f, 0.f);
			glBegin(GL_LINES);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(1000.f, 0.f, 0.f);
			glEnd();
			glColor3f(0.f, 1.f, 0.f);
			glBegin(GL_LINES);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 1000.f, 0.f);
			glEnd();
			glColor3f(0.f, 0.f, 1.f);
			glBegin(GL_LINES);
			glVertex3f(0.f, 0.f, 0.f);
			glVertex3f(0.f, 0.f, 1000.f);
			glEnd();
			glLineWidth(1.f);
			glColor3f(1.f, 1.f, 1.f);
			glEnable(GL_LIGHTING);
			glPopMatrix();
		}

#ifndef LINUX_BUILD
		glDisable(GL_LIGHTING);
		if(fionaConf.leapData.hand1.valid)
		{
			drawHand(fionaConf.leapData.hand1);
		}

		if(fionaConf.leapData.hand2.valid)
		{
			drawHand(fionaConf.leapData.hand2);
		}
	#endif
		glEnable(GL_LIGHTING);
	}

	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	if(rCount == 0)
	{
		//reset the scultping on the first frame..
		if( FionaIsFirstOfCycle() )
		{
			m_wbSys->reset();
			rCount = 1;
		}
	}

	//m_wbSys->fill(0.5f);

	//FionaScene::render();
}

void WorldBuilderScene::resize(int gwin, int w, int h)
{
	int i = findWin(gwin); if(i<0 ) return;
	wins[i].owin->resize(w,h);
#ifdef WIN32
	wins[i].owin->windowMovedOrResized ();
#endif
}

#define USE_TOGGLE_BUTTON 0

void WorldBuilderScene::setupScene(Ogre::SceneManager* scene)
{
	printf("%s\n", fionaConf.OgreMediaBasePath.c_str());
	//THIS NEEDS TO BE CALLED BEFORE BEING ABLE TO LOAD RESOURCES
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	
	//initialize default physics settings..
	physicsWorld = NxOgre::World::createWorld();
	physicsWorld->getRemoteDebugger()->connect();

	NxOgre::SceneDescription scene_description;
	scene_description.mGravity = NxOgre::Constants::MEAN_EARTH_GRAVITY;
	scene_description.mUseHardware = false;
  
	physicsScene = physicsWorld->createScene(scene_description);
	physicsScene->getMaterial(0)->setAll(0.1,0.9,0.5);
	NxOgre::PlaneGeometryDescription planeDesc(NxOgre::Vec3(0,1,0), -CAVE_CENTER.y);
	physicsScene->createSceneGeometry(planeDesc);

	critterRender = new Critter::RenderSystem(physicsScene, scene);

	NxOgre::ResourceSystem::getSingleton()->openProtocol(new Critter::OgreResourceProtocol());

	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) 
	{
		WorldBuilderScene::mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();
		// Load sounds
		if (mSoundManager->init())
		{
			printf("Initialized sound manager!\n");
			mSoundManager->createSound("click", "sounds_ogg/click.ogg", false, false, false);
			mSoundManager->createSound("cubes", "sounds_ogg/cubes.ogg", false, false, false);
			mSoundManager->createSound("delete", "sounds_ogg/delete.ogg", false, false, false);
			mSoundManager->createSound("delete_material", "sounds_ogg/delete_material.ogg", false, false, false);
			mSoundManager->createSound("delete_selection", "sounds_ogg/delete_selection.ogg", false, false, false);
			mSoundManager->createSound("duplicate", "sounds_ogg/duplicate.ogg", false, false, false);
			mSoundManager->createSound("erase", "sounds_ogg/erase.ogg", false, false, false);
			mSoundManager->createSound("paint", "sounds_ogg/paint.ogg", false, false, false);
			mSoundManager->createSound("position", "sounds_ogg/position.ogg", false, false, false);
			mSoundManager->createSound("rotate", "sounds_ogg/rotate.ogg", false, false, false);
			mSoundManager->createSound("scale", "sounds_ogg/scale.ogg", false, false, false);
			mSoundManager->createSound("sculpt_mode", "sounds_ogg/sculpt_mode.ogg", false, false, false);
			mSoundManager->createSound("select_nothing_pointed_at", "sounds_ogg/select_nothing_pointed_at.ogg", false, false, false);
			mSoundManager->createSound("spheres", "sounds_ogg/spheres.ogg", false, false, false);
			mSoundManager->createSound("world_mode", "sounds_ogg/world_mode.ogg", false, false, false);
			mSoundManager->createSound("bell", "sounds/bell.wav", false, false, false);
		}
	}

	//below two lines will turn on physics debugging
	//critterRender->setVisualisationMode(NxOgre::Enums::VisualDebugger_ShowAll);
	//physicsWorld->getVisualDebugger()->enable();
	
	if(!sceneName.empty())
	{
		DotSceneLoader loader;
		loader.setCritterRender(critterRender);
		std::string wbSceneName = sceneName;
		std::string dirToScene("..\\ext\\Media\\levels\\");
		dirToScene = dirToScene + wbSceneName + "\\" + wbSceneName + ".scene";
		loader.parseDotScene(dirToScene, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, scene);
	}

	//scene->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);
	/*scene->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
	scene->setShadowColour(Ogre::ColourValue(0.5, 0.5, 0.5));
	scene->setShadowTextureSize(1024);
	scene->setShadowTextureCount(1);*/
	//load up default plane, etc..
	Ogre::ColourValue background = Ogre::ColourValue(fionaConf.backgroundColor.x, fionaConf.backgroundColor.y, fionaConf.backgroundColor.z);
			
	//scene->setFog(Ogre::FOG_EXP, background, 0.001, 5000, 10000);
	
	scene->setSkyBox(true, "Examples/CloudyNoonSkyBox", 3000.f);  // set a skybox

	Ogre::MeshManager::getSingleton().createPlane("floor", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::Plane(Ogre::Vector3::UNIT_Y, 0), 1000, 1000, 1,1 , true, 1, 1, 1, Ogre::Vector3::UNIT_Z);

	Ogre::Entity* floor = scene->createEntity("Floor", "floor");
	floor->setMaterialName("Examples/GrassFloor");
	floor->setCastShadows(false);
	Ogre::SceneNode* planeNode = scene->getRootSceneNode()->createChildSceneNode();
	planeNode->setPosition(0.0, -1.4478, 0.0);
	planeNode->attachObject(floor);

	scene->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));

	Ogre::Light* light = scene->createLight();
	light->setType(Ogre::Light::LT_POINT);
	light->setPosition(-10, 40, 20);
	light->setSpecularColour(Ogre::ColourValue::White);

	Ogre::Light* secondLight = scene->createLight();
	secondLight->setType(Ogre::Light::LT_POINT);
	secondLight->setPosition(10, -40, 20);
	secondLight->setSpecularColour(Ogre::ColourValue::White);

	//makeBox(NxOgre::Matrix44(NxOgre::Vec3(0.0, 0.0, 0.0)), NxOgre::Vec3(0.0, 1.0, 0.0));
			
	//Ogre::SceneNode* headNode = scene->getRootSceneNode()->createChildSceneNode();
	//headNode->setPosition(0.f, 10.f, 3.f);
	//headNode->attachObject(scene->createEntity("Head", "ogrehead.mesh"));
	

	//setup actions.. eventually these will be read in from file..
	VRActionSet *pWorldMode = new VRActionSet();
	pWorldMode->SetName(std::string("world_mode"));
		
	VRSelect *pSelect = new VRSelect();
	//pSelect->SetButton(0);
	pSelect->SetButton(5);
	pSelect->SetOnRelease(true);
	pWorldMode->AddAction(pSelect);

	/*VRTSR *pTrans = new VRTSR();
	pTrans->SetNoMovement(true);
	pTrans->SetButton(5);
	pWorldMode->AddAction(pTrans);*/

	VRChangeTSR *pTSRChange = new VRChangeTSR();
	pTSRChange->SetButton(1);
	pTSRChange->SetOnRelease(true);
	pWorldMode->AddAction(pTSRChange);

	/*VRExportPly *pSave = new VRExportPly();
	pSave->SetButton(2);
	pSave->SetOnRelease(true);
	pWorldMode->AddAction(pSave);*/

	VRDelete *pDelete = new VRDelete();
	pDelete->SetButton(3);
	pDelete->SetOnRelease(true);
	pWorldMode->AddAction(pDelete);

	VRSwitchModes *pChange = new VRSwitchModes();
	pChange->SetButton(4);
	pChange->SetOnRelease(true);
	pWorldMode->AddAction(pChange);

	VRDuplicate *pDupe = new VRDuplicate();
	pDupe->SetButton(2);
	pDupe->SetOnRelease(true);
	pWorldMode->AddAction(pDupe);

	m_actions.AddSet(pWorldMode);

	//sculpt mode...
	VRSculptSet *pSculptMode = new VRSculptSet();
	pSculptMode->SetName(std::string("sculpt_mode"));

	VRResetSculpt *pReset = new VRResetSculpt();
	pReset->SetButton(3);
	pReset->SetOnRelease(true);
	pSculptMode->AddAction(pReset);

	VRSwitchModes *pChange2 = new VRSwitchModes();
	pChange2->SetButton(4);
	pChange2->SetOnRelease(true);
	pSculptMode->AddAction(pChange2);
#if USE_TOGGLE_BUTTON
	VRChangeBrush *pChangeBrush = new VRChangeBrush();
	pChangeBrush->SetButton(2);
	pChangeBrush->SetOnRelease(true);
	pSculptMode->AddAction(pChangeBrush);

	VRChangeTool *pChangeTool = new VRChangeTool();
	pChangeTool->SetButton(1);
	pChangeTool->SetOnRelease(true);
	pSculptMode->AddAction(pChangeTool);
#else
	VRChangeSpecificTool *pChangeErase = new VRChangeSpecificTool(WorldBuilderScene::ERASER);
	pChangeErase->SetButton(1);
	pChangeErase->SetOnRelease(true);
	pSculptMode->AddAction(pChangeErase);

	VRChangeSpecificTool *pChangePaint = new VRChangeSpecificTool(WorldBuilderScene::PAINT);
	pChangePaint->SetButton(0);
	pChangePaint->SetOnRelease(true);
	pSculptMode->AddAction(pChangePaint);

	VRChangeSpecificTool *pChangeSculpt = new VRChangeSpecificTool(WorldBuilderScene::SCULPTER);
	pChangeSculpt->SetButton(2);
	pChangeSculpt->SetOnRelease(true);
	pSculptMode->AddAction(pChangeSculpt);
#endif
	VRUseTool *pTools = new VRUseTool();
	pTools->SetNoMovement(true);
	pTools->SetButton(5);
	pTools->SetScenePtr(this);
	pSculptMode->AddAction(pTools);

	VRJoystickResize *pResize = new VRJoystickResize();
	pResize->SetScenePtr(this);

	pSculptMode->SetJoystickAction(pResize);	//action that we use for if user only presses the joystick (uses the action's JoystickMove function)
	pSculptMode->SetDrawAction(pTools);		//idle action that we use it's DrawCallback function for when no buttons or joystick are pressed

	m_actions.AddSet(pSculptMode);
	m_actions.SetCurrentSet(pSculptMode);

	printf("Successfully setup world builder scene!\n");
}

//test box
Critter::Body* WorldBuilderScene::makeBox(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3 & initialVelocity)
{
	Critter::BodyDescription bodyDescription;
	bodyDescription.mMass = 40.0f;
	bodyDescription.mLinearVelocity = initialVelocity;
  
	Critter::Body* box = critterRender->createBody(NxOgre::BoxDescription(1,1,1), globalPose, "cube.1m.mesh", bodyDescription);

	return box;
}

void WorldBuilderScene::tesselateCurrentVolume(float *data, float *redData, float *greenData, float *blueData, vec3i size, vec3i gridsize, float isolevel, std::vector<jvec3> & outVerts, std::vector<jvec3> & outColors, std::vector<jvec3> & outNormals)
{
	jvec3 gridStep=jvec3(2.0/(float)gridsize.x,2.0/(float)gridsize.y,2.0/(float)gridsize.z); //2 / 64 = 0.03125 world space

	vec3i dataGridStep(size.x/gridsize.x, size.y/gridsize.y, size.z/gridsize.z);	//currently 128/64 = 2 volume space

	jvec3 *verts=new jvec3[16];
	jvec3 *colors = new jvec3[16];

	int totalCount = 0;

	for(int k=0; k<gridsize.z-1; k++)
	{
		for(int j=0; j<gridsize.y-1; j++)
		{
			for(int i=0; i<gridsize.x-1; i++)
			{
				GridCell cell;
				jvec3 vcurf(i, j, k);
				vec3i vcuri(i, j, k);

				cell.pos[0]=jvec3(vcurf.x*gridStep.x, vcurf.y*gridStep.y, vcurf.z*gridStep.z)-jvec3(1.0f, 1.0f, 1.0f);
				
				//calc index to look up volume
				vec3i valPos0(vcuri.x*dataGridStep.x, vcuri.y*dataGridStep.y, vcuri.z*dataGridStep.z);
				
				int index = valPos0.x + valPos0.y*size.x + valPos0.z*size.x*size.y;
				cell.val[0]=data[index];
				cell.redVal[0] = redData[index];
				cell.greenVal[0] = greenData[index];
				cell.blueVal[0] = blueData[index];

				vec3i valPos;

				cell.pos[1]=cell.pos[0]+jvec3(gridStep.x, 0, 0);
				if(i==gridsize.x-1)
					valPos=valPos0;
				else
					valPos=valPos0+vec3i(dataGridStep.x, 0, 0);

				index = valPos.x + valPos.y*size.x + valPos.z*size.x*size.y;
				cell.val[1]=data[index];
				cell.redVal[1] = redData[index];
				cell.greenVal[1] = greenData[index];
				cell.blueVal[1] = blueData[index];

				cell.pos[2]=cell.pos[0]+jvec3(gridStep.x, gridStep.y, 0);
				valPos=valPos0+vec3i(i==gridsize.x-1 ? 0 : dataGridStep.x, j==gridsize.y-1 ? 0 : dataGridStep.y, 0);

				index = valPos.x + valPos.y*size.x + valPos.z*size.x*size.y;
				cell.val[2]=data[index];
				cell.redVal[2] = redData[index];
				cell.greenVal[2] = greenData[index];
				cell.blueVal[2] = blueData[index];

				cell.pos[3]=cell.pos[0]+jvec3(0, gridStep.y, 0);
				valPos=valPos0+vec3i(0, j==gridsize.y-1 ? 0 : dataGridStep.y, 0);

				index = valPos.x + valPos.y*size.x + valPos.z*size.x*size.y;
				cell.val[3]=data[index];
				cell.redVal[3] = redData[index];
				cell.greenVal[3] = greenData[index];
				cell.blueVal[3] = blueData[index];

				cell.pos[4]=cell.pos[0]+jvec3(0, 0, gridStep.z);
				valPos=valPos0+vec3i(0, 0, k==gridsize.z-1 ? 0 : dataGridStep.z);

				index = valPos.x + valPos.y*size.x + valPos.z*size.x*size.y;
				cell.val[4]=data[index];
				cell.redVal[4] = redData[index];
				cell.greenVal[4] = greenData[index];
				cell.blueVal[4] = blueData[index];

				cell.pos[5]=cell.pos[0]+jvec3(gridStep.x, 0, gridStep.z);
				valPos=valPos0+vec3i(i==gridsize.x-1 ? 0 : dataGridStep.x, 0, k==gridsize.z-1 ? 0 : dataGridStep.z);

				index = valPos.x + valPos.y*size.x + valPos.z*size.x*size.y;
				cell.val[5]=data[index];
				cell.redVal[5] = redData[index];
				cell.greenVal[5] = greenData[index];
				cell.blueVal[5] = blueData[index];

				cell.pos[6]=cell.pos[0]+jvec3(gridStep.x, gridStep.y, gridStep.z);
				valPos=valPos0+vec3i(i==gridsize.x-1 ? 0 : dataGridStep.x, j==gridsize.y-1 ? 0 : dataGridStep.y, k==gridsize.z-1 ? 0 : dataGridStep.z);

				index = valPos.x + valPos.y*size.x + valPos.z*size.x*size.y;
				cell.val[6]=data[index];
				cell.redVal[6] = redData[index];
				cell.greenVal[6] = greenData[index];
				cell.blueVal[6] = blueData[index];

				cell.pos[7]=cell.pos[0]+jvec3(0, gridStep.y, gridStep.z);
				valPos=valPos0+vec3i(0, j==gridsize.y-1 ? 0 : dataGridStep.y, k==gridsize.z-1 ? 0 : dataGridStep.z);

				index = valPos.x + valPos.y*size.x + valPos.z*size.x*size.y;
				cell.val[7]=data[index];
				cell.redVal[7] = redData[index];
				cell.greenVal[7] = greenData[index];
				cell.blueVal[7] = blueData[index];

				int numvert=polygonise(cell, isolevel, verts, colors);
				
				//kevins stupid normal calculotr
				//    7 - 6 
				//   /    /
				//  3 - 2  |
				//  |  4 | 5
				//  0  - 1
				//
				jvec3 normal = jvec3(0,0,0);
				//right - left
				normal.x = cell.val[6] - cell.val[7] +  cell.val[2] - cell.val[3] +  cell.val[5] - cell.val[4] +  cell.val[1] - cell.val[0];   
				//top - bottom
				normal.y = cell.val[7] - cell.val[4] +  cell.val[6] - cell.val[5] +  cell.val[3] - cell.val[0] +  cell.val[2] - cell.val[1];   
				//back - front
				normal.z = cell.val[7] - cell.val[3] +  cell.val[6] - cell.val[2] +  cell.val[4] - cell.val[0] +  cell.val[5] - cell.val[1];   

				//lets try flipping some of these because i did it backwards
				normal.z = -normal.z;
				normal.y = -normal.y;
				normal.x = -normal.x;

				//now we normliaze
				normal = normal.normalize();

				for(int n=0; n<numvert; n++)
				{
					outVerts.push_back(jvec3(verts[n].x, verts[n].y, verts[n].z));
					outColors.push_back(jvec3(colors[n].x, colors[n].y, colors[n].z));
					outNormals.push_back(normal);
					totalCount++;
				}
			}
		}
	}

	delete[] verts;
	delete[] colors;
}

void WorldBuilderScene::initMarchingCube(void)
{
	vp->setBackgroundColour(Ogre::ColourValue(fionaConf.backgroundColor.x,
											fionaConf.backgroundColor.y,
											fionaConf.backgroundColor.z));
	glShadeModel (GL_SMOOTH);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	//Form multi-face view
	glDisable(GL_CULL_FACE);

	glDepthMask(true);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);

	//showFPS(true);

	//Glew extension manager initialisation
	//glewInit();	//FIONAUT - we do this earlier in FionaUTWin32.cpp..

	//Print Avalaible OpenGL extensions
	std::cout<<glGetString(GL_EXTENSIONS)<<"\n";

//Disable Vsync on windows plateform
#ifdef WIN32
	//wglSwapIntervalEXT(0);	//FIONAUT - turned this off as we need to vsync in cave..
#endif

	srand(time(0));


	/////GLSL/////

	//Program object creation
	programObject = glCreateProgram();


	////Shaders loading////
	//Geometry Shader loading
	initShader(programObject, "Shaders/TestG80_GS2.glsl", GL_GEOMETRY_SHADER);
	//Geometry Shader require a Vertex Shader to be used
	initShader(programObject, "Shaders/TestG80_VS.glsl", GL_VERTEX_SHADER);
	//Fragment Shader for per-fragment lighting
	initShader(programObject, "Shaders/TestG80_FS.glsl", GL_FRAGMENT_SHADER);
	////////


	//Get max number of geometry shader output vertices
	GLint temp;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES,&temp);
	std::cout<<"Max GS output vertices:"<<temp<<"\n";
	
	////Setup Geometry Shader////
	//Set POINTS primitives as INPUT
    /*glProgramParameteri(programObject,GL_GEOMETRY_INPUT_TYPE , GL_POINTS );
	//Set TRIANGLE STRIP as OUTPUT
	glProgramParameteri(programObject,GL_GEOMETRY_OUTPUT_TYPE , GL_TRIANGLE_STRIP);
	//Set maximum number of vertices to be generated by Geometry Shader to 16
	//16 is the maximum number of vertices a marching cube configuration can own
	//This parameter is very important and have an important impact on Shader performances
	//Its value must be chosen closer as possible to real maximum number of vertices
	glProgramParameteri(programObject,GL_GEOMETRY_VERTICES_OUT_EXT, 16);
	*/

	//Link whole program object (Geometry+Vertex+Fragment)
	glLinkProgram(programObject);
	//Test link success
	GLint ok = false;
	glGetProgramiv(programObject, GL_LINK_STATUS, &ok);
	if (!ok){
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetProgramInfoLog(programObject, maxLength, &maxLength, infoLog);
		std::cout<<"Link error: "<<infoLog<<"\n";
		delete []infoLog;
	}

    //Program validation
    glValidateProgram(programObject);
    ok = false;
	glGetProgramiv(programObject, GL_VALIDATE_STATUS, &ok);
    if (!ok){
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetProgramInfoLog(programObject, maxLength, &maxLength, infoLog);
		std::cout<<"Validation error: "<<infoLog<<"\n";
		delete []infoLog;
	}

	//Bind program object for parameters setting
	glUseProgram(programObject);

	////Textures generation////

	//Edge Table texture//
	//This texture store the 256 different configurations of a marching cube.
	//This is a table accessed with a bitfield of the 8 cube edges states 
	//(edge cut by isosurface or totally in or out).
	//(cf. MarchingCubes.cpp)

	glGenTextures(1, &(this->edgeTableTex));
	glActiveTexture(GL_TEXTURE1);
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, this->edgeTableTex);
	//Integer textures must use nearest filtering mode
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//We create an integer texture with new GL_EXT_texture_integer formats
	glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA16I_EXT, 256, 1, 0, GL_ALPHA_INTEGER_EXT, GL_INT, &edgeTable);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, 256, 1, 0, GL_RED_INTEGER, GL_INT, &edgeTable);


	//Triangle Table texture//
	//This texture store the vertex index list for 
	//generating the triangles of each configurations.
	//(cf. MarchingCubes.cpp)
	
	//START - causes ATI cards to not work (b/c of texelFetch2D command)
	glGenTextures(1, &(this->triTableTex));
	glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, this->triTableTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA16I_EXT, 16, 256, 0, GL_ALPHA_INTEGER_EXT, GL_INT, &triTable);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_R32I, 16, 256, 0, GL_RED_INTEGER, GL_INT, &triTable);
	//END - causes ATI cards to not work (b/c of texelFetch2D command)

	//make the data 
	
	//Datafield//
	//Store the volume data to polygonise

	//Set current texture//
	glActiveTexture(GL_TEXTURE0);
	//glActiveTexture(GL_TEXTURE1);
	//glActiveTexture(GL_TEXTURE2);
	glEnable(GL_TEXTURE_3D);

	// create WorldBuilder Volume
	//cgContext = cgCreateContext();
	
	m_wbSys = new WorldBuilderSystem(cgContext, dataSize.x, 
		dataSize.y, dataSize.z, 2, GL_RGBA32F, GL_RGBA, GL_FLOAT);
	
	std::cout << "After create world builder system: " << gluErrorString(glGetError()) << "\n";

	//m_wbRenderer = new VolumeRender(cgContext, m_wbSys->getStateBuffer());
	
	m_wbSys->createLog(m_loggingDirectory);

	glUseProgram(programObject);
	glBindTexture(GL_TEXTURE_3D, m_wbSys->getStateBuffer()->getTexture());
	
	////Samplers assignment///
	glUniform1i(glGetUniformLocation(programObject, "dataFieldTex"), 0);
	glUniform1i(glGetUniformLocation(programObject, "edgeTableTex"), 1); 
	glUniform1i(glGetUniformLocation(programObject, "triTableTex"), 2); 

	////Uniforms parameters////
	//Initial isolevel
	glUniform1f(glGetUniformLocation(programObject, "isolevel"), isolevel); 
	//Step in data 3D texture for gradient computation (lighting)
	glUniform3f(glGetUniformLocation(programObject, "dataStep"), 1.0f/dataSize.x, 1.0f/dataSize.y, 1.0f/dataSize.z); 

	//Decal for each vertex in a marching cube
	//glm::vec3 vertDecals[8] = { glm::vec3(0.f, 0.f, 0.f), glm::vec3(cubeStep.x, 0.f, 0.f), glm::vec3(cubeStep.x, cubeStep.y, 0.f), glm::vec3(0.f, cubeStep.y, 0.f),
	//	glm::vec3(0.f, 0.f, cubeStep.z), glm::vec3(cubeStep.x, 0.f, cubeStep.z), glm::vec3(cubeStep.x, cubeStep.y, cubeStep.z), glm::vec3(0.f, cubeStep.y, cubeStep.z) };

	//glUniform3fv(glGetUniformLocation(programObject, "vertDecals"), 8, glm::value_ptr(vertDecals[0]));

	glUniform3f(glGetUniformLocation(programObject, "vertDecals[0]"), 0.0f, 0.0f, 0.0f); 
	glUniform3f(glGetUniformLocation(programObject, "vertDecals[1]"), cubeStep.x, 0.0f, 0.0f); 
	glUniform3f(glGetUniformLocation(programObject, "vertDecals[2]"), cubeStep.x, cubeStep.y, 0.0f); 
	glUniform3f(glGetUniformLocation(programObject, "vertDecals[3]"), 0.0f, cubeStep.y, 0.0f); 
	glUniform3f(glGetUniformLocation(programObject, "vertDecals[4]"), 0.0f, 0.0f, cubeStep.z); 
	glUniform3f(glGetUniformLocation(programObject, "vertDecals[5]"), cubeStep.x, 0.0f, cubeStep.z); 
	glUniform3f(glGetUniformLocation(programObject, "vertDecals[6]"), cubeStep.x, cubeStep.y, cubeStep.z); 
	glUniform3f(glGetUniformLocation(programObject, "vertDecals[7]"), 0.0f, cubeStep.y, cubeStep.z); 

	////////////////////////////////////////////////////////////////////////////////////////////

	///////////////
	//Final error testing//
	std::cout<<"Init end error: "<<gluErrorString(glGetError())<<"\n";
	
	////Light source configuration////
	GLfloat LightAmbient[]= { 0.01f, 0.01f, 0.01f, 1.0f };
	GLfloat LightDiffuse[]= { 0.1f, 0.1f, 0.1f, 1.0f };	
	GLfloat LightPosition[]= { 5.0f, 5.0f, 5.0f, 1.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);	
	glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);	
	glLightfv(GL_LIGHT0, GL_SPECULAR, LightDiffuse);	
	glLightfv(GL_LIGHT0, GL_POSITION,LightPosition);	
	glEnable(GL_LIGHT0);

	//////Grid data construction
	//Linear Walk
	gridData=new float[cubeSize.x*cubeSize.y*cubeSize.z*3];
	int ii=0;
  for(float k=-1; k<1.0f; k+=cubeStep.z)
  for(float j=-1; j<1.0f; j+=cubeStep.y)
  for(float i=-1; i<1.0f; i+=cubeStep.x){
        gridData[ii]= i;	
        gridData[ii+1]= j;
        gridData[ii+2]= k;

        ii+=3;
	}


	//VBO configuration for marching grid linear walk
	glGenVertexArrays(1, &gridDataVAO);
	glGenBuffers(1, &gridDataBuffId);
	glBindVertexArray(gridDataVAO);
	glBindBuffer(GL_ARRAY_BUFFER, gridDataBuffId);
	glBufferData(GL_ARRAY_BUFFER, cubeSize.x*cubeSize.y*cubeSize.z*3*sizeof(float), gridData, GL_STATIC_DRAW);
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//Swizzled Walk 
	int n=0;
	swizzledWalk(n, gridData, vec3i(0,0,0), vec3i(cubeSize.x, cubeSize.y, cubeSize.z), cubeSize);

	//VBO configuration for marching grid Swizzled walk
	glGenBuffers(1, &gridDataSwizzledBuffId);
	glBindBuffer(GL_ARRAY_BUFFER, gridDataSwizzledBuffId);
	glBufferData(GL_ARRAY_BUFFER, cubeSize.x*cubeSize.y*cubeSize.z*3*4, gridData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glUseProgram(NULL);

	glDisable(GL_TEXTURE_3D);
}


void WorldBuilderScene::initShader(GLuint programObject, const char *filen, GLuint type)
{
	//Source file reading
	std::string buff;
	std::ifstream file;
	std::string filename=filen;
	std::cerr.flush();
	file.open(filename.c_str());
	std::string line;
	while(std::getline(file, line))
		buff += line + "\n";

	const GLchar *txt=buff.c_str();

	//Shader object creation
	GLuint object = glCreateShader(type);
	
	//Source code assignment
	glShaderSource(object, 1, &txt, NULL);

	//Compile shader object
	glCompileShader(object);

	//Check if shader compiled
	/*GLint ok = 0;
	glGetProgramiv(object, GL_COMPILE_STATUS, &ok);
	if (!ok){
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetInfoLogARB(object, maxLength, &maxLength, infoLog);
		std::cout<<"Compilation error: "<<infoLog<<"\n";
		delete []infoLog;
	}*/

	// attach shader to program object
	glAttachShader(programObject, object);

	// delete object, no longer needed
	glDeleteShader(object);

	//Global error checking
	std::cout<<"InitShader: "<<filen<<" Errors:"<<gluErrorString(glGetError())<<"\n";
}

void WorldBuilderScene::setTool(unsigned int tool)
{
	if(m_currentTool == tool)
	{
		//if already in sculpt mode, press again to toggle add type..
		if(m_currentTool == WorldBuilderScene::SCULPTER)
		{
			m_addMode++;
			m_addMode = m_addMode % WorldBuilderScene::NUM_ADD_MODES;
		}
	}

	m_currentTool = tool;
	
	if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED) && FionaIsFirstOfCycle()) 
	{
		switch (m_currentTool) 
		{
			case ERASER:
				playSound("erase");
				break;
			case PAINT:
				playSound("paint");
				break;
			case SCULPTER:
				// Play the appropriate sound based on which sculpt mode we're in
				switch (m_addMode) {
				case BLOB:
					playSound("spheres");
					break;
				case CUBE:
					playSound("cubes");
					break;
				}
				break;
		}
	}
}

void WorldBuilderScene::swizzledWalk(int &n, float *gridData, vec3i pos, vec3i size, const jvec3 &cubeSize)
{
	if(size.x>1){
		vec3i newSize=size/2;

		swizzledWalk(n, gridData, pos, newSize, cubeSize);
		swizzledWalk(n, gridData, pos+vec3i(newSize.x, 0, 0), newSize, cubeSize);
		swizzledWalk(n, gridData, pos+vec3i(0, newSize.y,0), newSize, cubeSize);
		swizzledWalk(n, gridData, pos+vec3i(newSize.x, newSize.y, 0), newSize, cubeSize);

		swizzledWalk(n, gridData, pos+vec3i(0, 0, newSize.z), newSize, cubeSize);
		swizzledWalk(n, gridData, pos+vec3i(newSize.x, 0, newSize.z), newSize, cubeSize);
		swizzledWalk(n, gridData, pos+vec3i(0, newSize.y, newSize.z), newSize, cubeSize);
		swizzledWalk(n, gridData, pos+vec3i(newSize.x, newSize.y, newSize.z), newSize, cubeSize);
	}else{
		gridData[n]=(pos.x/cubeSize.x)*2.0f-1.0f;
		gridData[n+1]=(pos.y/cubeSize.y)*2.0f-1.0f;
		gridData[n+2]=(pos.z/cubeSize.z)*2.0f-1.0f;
		n+=3;
	}
}

void WorldBuilderScene::getSecondTrackerPos(jvec3 &vOut, const jvec3 &vOffset) const
{
	FionaScene::getSecondTrackerWorldSpace(vOut);
	//vOut += vOffset;
	float toDeg = m_wiiFitRotation * PI/180;
    mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
    //vOut = m.transpose().inv()  * vOut;
	vOut = m  * vOut;
}

jvec3 WorldBuilderScene::getWandPos(float x_off, float y_off, float z_off) const {
    jvec3 vPos;
    getWandWorldSpace(vPos);
    vPos.x += x_off; vPos.y += y_off; vPos.z += z_off;

    float toDeg = m_wiiFitRotation * PI/180;
    mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
    //vPos = m.transpose().inv()  * vPos;
	//jvec3 vOffset(x_off, y_off, z_off);
	//vOffset = m.transpose().inv() * vOffset;
	vPos = m * vPos;

    return vPos;
}


jvec3 WorldBuilderScene::getNormalizedPos(const jvec3 &vPos) const 
{
    jvec3 pos = jvec3(m_cubeScale, m_cubeScale, m_cubeScale);
    pos.x += vPos.x; pos.y += vPos.y; pos.z += vPos.z;
    pos /= (m_cubeScale*2);

    return pos;
}

jvec3 WorldBuilderScene::getNormalizedWandPos(float x_off, float y_off, float z_off) const 
{
    return this->getNormalizedPos(this->getWandPos(x_off, y_off, z_off));
}

void WorldBuilderScene::getNormalizedSecondTrackerPos(jvec3 &vOut, const jvec3 &vOffset) const
{
	getSecondTrackerPos(vOut);
    
	vOut += vOffset;
	vOut += jvec3(m_cubeScale, m_cubeScale, m_cubeScale);
    vOut /= (m_cubeScale*2);
}

bool WorldBuilderScene::inBounds(const jvec3& pos) {
    if (pos.x >= 0.0f && pos.y >= 0.0f && pos.z >= 0.0f &&
    pos.x <= 1.0f && pos.y <= 1.0f && pos.z <= 1.0f) {
        return true;
    } return false;
}

void WorldBuilderScene::updateLeap(void)
{
	
}

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/io.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>

void WorldBuilderScene::renderMarchingCubes(void)
{
	if(wiiFitPressed())
	{
		m_wiiFitRotation = m_wiiFitRotation + WII_FIT_ROTATION_SPEED;
		if(m_wiiFitRotation > 360.f)
		{
			m_wiiFitRotation = 0.f;
		}
		//printf("%f\n", m_wiiFitRotation);
	}

	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_3D);

	// volume bounding wireframe
	if (m_cubeWireFrame) 
	{
		glPushMatrix();
		glDisable(GL_LIGHTING);
		glColor3f(1.0f, 1.0f, 1.0f);
		glutWireCube(m_cubeScale*2.0f);
		glEnable(GL_LIGHTING);
		glPopMatrix();
	}                                                        

	bool doWiiFitRotation = false;
	//begin wii-fit rotation
	if(doWiiFitRotation)
	{
		//rotate first before drawing the indicators...
		glPushMatrix();
		glRotatef(m_wiiFitRotation, 0.f, 1.f, 0.f);
	}

	// draw the two indicators
	if(VRActionSet *pSet = m_actions.GetCurrentSet())
	{
		VRAction *pAction = pSet->GetCurrentAction();
		if(pAction != 0)
		{
			if(pAction->GetType() == VRAction::WAND)
			{
				static_cast<VRWandAction*>(pAction)->DrawCallback();
			}
		}
		else
		{
			//if no action - draw idle function
			m_actions.GetCurrentSet()->IdleDrawCallback();
		}
	}

	//glEnable(GL_DEPTH_TEST);
   	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_3D);
   
	glDepthMask(GL_TRUE);

	glDisable(GL_STENCIL_TEST);
	glDisable(GL_ALPHA_TEST);

	//start fionaut
	glColor4f(cosf(isolevel*10.0-0.5), sinf(isolevel*10.0-0.5), cosf(1.0-isolevel),1.0);

	//Shader program binding
	glUseProgram(programObject);

	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, m_wbSys->getStateBuffer()->getTexture(m_wbSys->m_current));
	glUniform1i(glGetUniformLocation(programObject, "dataFieldTex"), 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, edgeTableTex);
	glUniform1i(glGetUniformLocation(programObject, "edgeTableTex"), 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, triTableTex);
	glUniform1i(glGetUniformLocation(programObject, "triTableTex"), 2);

	//Current isolevel uniform parameter setting
	glUniform1f(glGetUniformLocation(programObject, "isolevel"), isolevel); 

	//glEnable(GL_LIGHTING);
	//Switch to wireframe or solid rendering mode
	if(wireframe)
		glPolygonMode(GL_FRONT_AND_BACK , GL_LINE );
	else
		glPolygonMode(GL_FRONT_AND_BACK , GL_FILL );

	if(!enableVBO){
		//Initial geometries are points. One point is generated per marching cube.
		if (!enableDisplayList)
		{
			glScalef(m_cubeScale, m_cubeScale, m_cubeScale);
			glBegin(GL_POINTS);
				float tmp = 1.0;
				for(float k=-tmp; k<tmp; k+=cubeStep.z)
				for(float j=-tmp; j<tmp; j+=cubeStep.y)
				for(float i=-tmp; i<tmp; i+=cubeStep.x){
					glVertex3f(i, j, k);	
				}
			glEnd();
		}
		else
		{
			//if we haven't made the display list, go and make it
			if (!m_DisplayList)
			{
				m_DisplayList = glGenLists(1);
				glNewList(m_DisplayList, GL_COMPILE);
					glScalef(m_cubeScale, m_cubeScale, m_cubeScale);
					glBegin(GL_POINTS);
						float tmp = 1.0;
						for(float k=-tmp; k<tmp; k+=cubeStep.z)
						for(float j=-tmp; j<tmp; j+=cubeStep.y)
						for(float i=-tmp; i<tmp; i+=cubeStep.x){
							glVertex3f(i, j, k);	
						}
					glEnd();
				glEndList();
			}
			else
			{
				float m[16];
				float p[16];
				glGetFloatv(GL_MODELVIEW_MATRIX, m);
				glGetFloatv(GL_PROJECTION_MATRIX, p);

				glm::mat4 viewMatrix = glm::make_mat4(m);
				glm::mat4 projectionMatrix = glm::make_mat4(p);

				glm::mat4 mvp = projectionMatrix * viewMatrix;

				glUniformMatrix4fv(glGetUniformLocation(programObject, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

				//call the list
				glCallList(m_DisplayList);
			}
		}
	}else{
		///VBO
		glScalef(m_cubeScale, m_cubeScale, m_cubeScale);
		
		float m[16];
		float p[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, m);
		glGetFloatv(GL_PROJECTION_MATRIX, p);

		glm::mat4 viewMatrix = glm::make_mat4(m);
		glm::mat4 projectionMatrix = glm::make_mat4(p);

		glm::mat4 mvp = projectionMatrix * viewMatrix;

		glUniformMatrix4fv(glGetUniformLocation(programObject, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));

		glBindVertexArray(gridDataVAO);
		glDrawArrays(GL_POINTS, 0, cubeSize.x*cubeSize.y*cubeSize.z);
		glBindVertexArray(0);
		/*if(enableSwizzledWalk)
			glBindBuffer(GL_ARRAY_BUFFER, gridDataSwizzledBuffId);
		else
			glBindBuffer(GL_ARRAY_BUFFER, gridDataBuffId);

		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0,  NULL);
		glDrawArrays(GL_POINTS, 0, cubeSize.x*cubeSize.y*cubeSize.z);
		glDisableClientState(GL_VERTEX_ARRAY);

		glBindBuffer(GL_ARRAY_BUFFER, 0);*/
	}

	//glDisable(GL_LIGHTING);

	//glBindTexture(GL_TEXTURE_3D, 0);
	glUseProgram(NULL);
	//glDisable(GL_TEXTURE_3D);

	if(doWiiFitRotation)
	{
		glPopMatrix();
	}
}

int WorldBuilderScene::edgeTable[256]={
	0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
	0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
	0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
	0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
	0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
	0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
	0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
	0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
	0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
	0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
	0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
	0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
	0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
	0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
	0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
	0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
	0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
	0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
	0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
	0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
	0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
	0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
	0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
	0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
	0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
	0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
	0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
	0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
	0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
	0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
	0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
	0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   };

int WorldBuilderScene::triTable[256][16] =
	{{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
	{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
	{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
	{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
	{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
	{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
	{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
	{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
	{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
	{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
	{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
	{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
	{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
	{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
	{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
	{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
	{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
	{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
	{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
	{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
	{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
	{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
	{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
	{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
	{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
	{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
	{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
	{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
	{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
	{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
	{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
	{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
	{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
	{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
	{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
	{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
	{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
	{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
	{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
	{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
	{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
	{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
	{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
	{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
	{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
	{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
	{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
	{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
	{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
	{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
	{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
	{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
	{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
	{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
	{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
	{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
	{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
	{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
	{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
	{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
	{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
	{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
	{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
	{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
	{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
	{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
	{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
	{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
	{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
	{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
	{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
	{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
	{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
	{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
	{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
	{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
	{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
	{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
	{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
	{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
	{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
	{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
	{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
	{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
	{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
	{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
	{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
	{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
	{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
	{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
	{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
	{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
	{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
	{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
	{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
	{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
	{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
	{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
	{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
	{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
	{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
	{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
	{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
	{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
	{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
	{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
	{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
	{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
	{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
	{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
	{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
	{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
	{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
	{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
	{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
	{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
	{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
	{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
	{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
	{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
	{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
	{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
	{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
	{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
	{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
	{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
	{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
	{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
	{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};
 
/*
   Linearly interpolate the position where an isosurface cuts
   an edge between two vertices, each with their own scalar value
*/
jvec3 WorldBuilderScene::vertexInterp(float isolevel, jvec3 p1, jvec3 p2, float valp1, float valp2, float & mu) {
   jvec3 p;

  /* if (fabs(isolevel-valp1) < 0.00001)
      return(p1);
   if (fabs(isolevel-valp2) < 0.00001)
      return(p2);
   if (fabs(valp1-valp2) < 0.00001)
      return(p1);*/

   mu = (isolevel - valp1) / (valp2 - valp1);
   p = p1 + (p2 - p1) * mu;

   return(p);
}

int WorldBuilderScene::polygonise(GridCell &grid, float isolevel, jvec3 *triangles, jvec3 *colors)
{
   int i,ntriang;
   int cubeindex;
   jvec3 vertlist[12];
   jvec3 colorList[12];

   /*
      Determine the index into the edge table which
      tells us which vertices are inside of the surface
   */
   cubeindex = 0;
   if (grid.val[0] < isolevel) cubeindex |= 1;
   if (grid.val[1] < isolevel) cubeindex |= 2;
   if (grid.val[2] < isolevel) cubeindex |= 4;
   if (grid.val[3] < isolevel) cubeindex |= 8;
   if (grid.val[4] < isolevel) cubeindex |= 16;
   if (grid.val[5] < isolevel) cubeindex |= 32;
   if (grid.val[6] < isolevel) cubeindex |= 64;
   if (grid.val[7] < isolevel) cubeindex |= 128;

   //for colors we need to interpolate the actual red/green/blue values...

   /* Cube is entirely in/out of the surface */
   if (edgeTable[cubeindex] == 0)
      return(0);

   float mu = 0.f;
   /* Find the vertices where the surface intersects the cube */
   if (edgeTable[cubeindex] & 1)
   {
	   int i1 = 0;
	   int i2 = 1;
       vertlist[0] = vertexInterp(isolevel,grid.pos[0],grid.pos[1],grid.val[0],grid.val[1], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[0] = c1 + (c2 - c1) * mu;
   }
   
   if (edgeTable[cubeindex] & 2)
   {
	   int i1 = 1;
	   int i2 = 2;
	   vertlist[1] = vertexInterp(isolevel,grid.pos[1],grid.pos[2],grid.val[1],grid.val[2], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[1] = c1 + (c2 - c1) * mu;
   }

   if (edgeTable[cubeindex] & 4)
   {
	   int i1 = 2;
	   int i2 = 3;
       vertlist[2] = vertexInterp(isolevel,grid.pos[2],grid.pos[3],grid.val[2],grid.val[3], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[2] = c1 + (c2 - c1) * mu;
   }
   
   if (edgeTable[cubeindex] & 8)
   {
	   int i1 = 3;
	   int i2 = 0;
       vertlist[3] = vertexInterp(isolevel,grid.pos[3],grid.pos[0],grid.val[3],grid.val[0], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[3] = c1 + (c2 - c1) * mu;
   }
   
   if (edgeTable[cubeindex] & 16)
   {
	   int i1 = 4;
	   int i2 = 5;
       vertlist[4] = vertexInterp(isolevel,grid.pos[4],grid.pos[5],grid.val[4],grid.val[5], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[4] = c1 + (c2 - c1) * mu;
   }
   
   if (edgeTable[cubeindex] & 32)
   {
	   int i1 = 5;
	   int i2 = 6;
       vertlist[5] = vertexInterp(isolevel,grid.pos[5],grid.pos[6],grid.val[5],grid.val[6], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[5] = c1 + (c2 - c1) * mu;
   }
   
   if (edgeTable[cubeindex] & 64)
   {
	   int i1 = 6;
	   int i2 = 7;
       vertlist[6] = vertexInterp(isolevel,grid.pos[6],grid.pos[7],grid.val[6],grid.val[7], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[6] = c1 + (c2 - c1) * mu;
   }

   if (edgeTable[cubeindex] & 128)
   {
	   int i1 = 7;
	   int i2 = 4;
       vertlist[7] = vertexInterp(isolevel,grid.pos[7],grid.pos[4],grid.val[7],grid.val[4], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[7] = c1 + (c2 - c1) * mu;
   }
   
   if (edgeTable[cubeindex] & 256)
   {
	   int i1 = 0;
	   int i2 = 4;
       vertlist[8] = vertexInterp(isolevel,grid.pos[0],grid.pos[4],grid.val[0],grid.val[4], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[8] = c1 + (c2 - c1) * mu;
   }

   if (edgeTable[cubeindex] & 512)
   {
	   int i1 = 1;
	   int i2 = 5;
       vertlist[9] = vertexInterp(isolevel,grid.pos[1],grid.pos[5],grid.val[1],grid.val[5], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[9] = c1 + (c2 - c1) * mu;
   }
   
   if (edgeTable[cubeindex] & 1024)
   {
	   int i1 = 2;
	   int i2 = 6;
       vertlist[10] = vertexInterp(isolevel,grid.pos[2],grid.pos[6],grid.val[2],grid.val[6], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[10] = c1 + (c2 - c1) * mu;
   }

   if (edgeTable[cubeindex] & 2048)
   {
	   int i1 = 3;
	   int i2 = 7;
       vertlist[11] = vertexInterp(isolevel,grid.pos[3],grid.pos[7],grid.val[3],grid.val[7], mu);
	   jvec3 c1(grid.redVal[i1], grid.greenVal[i1], grid.blueVal[i1]);
	   jvec3 c2(grid.redVal[i2], grid.greenVal[i2], grid.blueVal[i2]);
	   colorList[11] = c1 + (c2 - c1) * mu;
   }

   /* Create the triangle */
   ntriang = 0;
   for (i=0;triTable[cubeindex][i]!=-1;i+=3) {
      triangles[ntriang] = vertlist[triTable[cubeindex][i  ]];
      triangles[ntriang+1] = vertlist[triTable[cubeindex][i+1]];
      triangles[ntriang+2] = vertlist[triTable[cubeindex][i+2]];
	  colors[ntriang] = colorList[triTable[cubeindex][i]];
	  colors[ntriang+1] = colorList[triTable[cubeindex][i+1]];
	  colors[ntriang+2] = colorList[triTable[cubeindex][i+2]];
      ntriang+=3;
   }

   return(ntriang);
}

void WorldBuilderScene::makeMesh(float *geomData, float *redData, float *greenData, float *blueData)
{
	std::vector<jvec3> verts;
	std::vector<jvec3> colors;
	std::vector<int> indices;
	std::vector<int> goodIndices;
	std::vector<jvec3> normals;

	//tesselateCurrentVolume(geomData, redData, greenData, blueData, vec3i(128,128,128), vec3i(cubeSize.x, cubeSize.y, cubeSize.z), isolevel, verts, colors, normals);
	tesselateCurrentVolume(geomData, redData, greenData, blueData, vec3i(128,128,128), vec3i(128,128,128), isolevel, verts, colors, normals);

	static const float fEPSILON = 0.005f;

	//step 1, let's build a list of unique vertices..
	std::vector<jvec3> uniqueVerts;
	std::vector<jvec3> uniqueColors;
	std::vector<jvec3> uniqueNormals;

	int totalCount = 0;
	int overallCount = 0;

	bool bFlip = false;
	std::vector<jvec3>::const_iterator v1 = verts.begin();
	while(v1 != verts.end())
	{
		int fIndex = 0;
		bool found = false;
		//look through our list of unique verts to see if the current vertex is already present
		std::vector<jvec3>::const_iterator v2 = uniqueVerts.begin();
		while(v2 != uniqueVerts.end())
		{
			if(fabs(v1->x - v2->x) < fEPSILON && fabs(v1->y - v2->y) < fEPSILON && fabs(v1->z - v2->z) < fEPSILON)
			{
				//if it is, push the existing index and accumulate the face normal to what will be the vertex normal
				indices.push_back(fIndex);
				uniqueNormals[fIndex] += normals[overallCount];
				found = true;
				break;
			}
			
			fIndex++;
			v2++;
		}

		if(!found)
		{
			//if not found, add to unique list
			uniqueVerts.push_back(*v1);
			uniqueColors.push_back(colors[overallCount]);
			uniqueNormals.push_back(normals[overallCount]);
			indices.push_back(totalCount);
			totalCount++;
		}
		overallCount++;
		v1++;
	}

	for(unsigned int n = 0; n < indices.size(); n+=3)
	{
		if((indices[n] == indices[n+1]) || (indices[n]==indices[n+2]) || (indices[n+1]==indices[n+2]))
		{

		}
		else
		{
			goodIndices.push_back(indices[n]);
			goodIndices.push_back(indices[n+1]);
			goodIndices.push_back(indices[n+2]);
		}
	}

	indices = goodIndices;

	int numNormals = uniqueNormals.size();
	for(int n = 0; n < numNormals; ++n)
	{
		uniqueNormals[n] = uniqueNormals[n].normalize();
	}

	std::vector<jvec3> faceNormals;
	//walk through new unique indices / verts and calculate face normals
	std::vector<int> flipIndices;
	for(unsigned int i = 0; i < indices.size(); i+=3)
	{
		jvec3 v = (uniqueVerts[indices[i]]-uniqueVerts[indices[i+1]])*1000.f;
		jvec3 v2 = (uniqueVerts[indices[i]]-uniqueVerts[indices[i+2]])*1000.f;
		v = v.normalize();
		v2 = v2.normalize();
		jvec3 faceNorm = (v*v2).normalize();
		if(faceNorm % uniqueNormals[indices[i]] < 0.f)
		{
			flipIndices.push_back(i);
		}
		//faceNormals.push_back();
	}

	for(unsigned int i = 0; i < flipIndices.size(); i++)
	{
		int temp = indices[flipIndices[i]+2];
		indices[flipIndices[i]+2] = indices[flipIndices[i]+1];
		indices[flipIndices[i]+1] = temp;
	}

	/*std::vector<jvec3> vertexNormals;
	std::vector<int> vertexNormalCount;
	vertexNormals.resize(uniqueVerts.size());
	vertexNormalCount.resize(uniqueVerts.size());
	int faceIndex = 0;
	for(unsigned int i = 0; i < indices.size(); i+=3)
	{
		vertexNormals[indices[i]] += faceNormals[faceIndex];
		vertexNormals[indices[i+1]] += faceNormals[faceIndex];
		vertexNormals[indices[i+2]] += faceNormals[faceIndex];
		vertexNormalCount[indices[i]]++;
		vertexNormalCount[indices[i+1]]++;
		vertexNormalCount[indices[i+2]]++;

		faceIndex++;
	}

	//run through our normals and average them..
	int numNormals = vertexNormals.size();
	for(int n = 0; n < numNormals; ++n)
	{
		vertexNormals[n] = vertexNormals[n] / (float)vertexNormalCount[n];
	}*/
	
	//don't try to make a mesh if we don't have any data...
	if(uniqueVerts.size() > 0)
	{
		static int meshCount = 0;
		char name[256];
		memset(name, 0, sizeof(name));
		const char *sName = "sculptedMesh";
		sprintf(name, "%s_%d", sName, meshCount);
		Ogre::ManualObject *testMesh = scene->createManualObject(Ogre::String(name));
	
		if(testMesh)
		{
			testMesh->estimateVertexCount(uniqueVerts.size());
	
			testMesh->begin("Simple_Perpixel"/*"Examples/Sculpted"*/, Ogre::RenderOperation::OT_TRIANGLE_LIST);
			std::vector<jvec3>::const_iterator vi = uniqueVerts.begin();
			int c = 0;
			while(vi != uniqueVerts.end())
			{
				testMesh->position(vi->x, vi->y, vi->z);
				testMesh->normal(uniqueNormals[c].x, uniqueNormals[c].y, uniqueNormals[c].z);//vertexNormals[c].x, vertexNormals[c].y, vertexNormals[c].z);
				testMesh->colour(uniqueColors[c].x, uniqueColors[c].y, uniqueColors[c].z);
				testMesh->textureCoord((uniqueNormals[c].x * 0.5f) + 0.5f, (uniqueNormals[c].y * 0.5f) + 0.5f);
				c++;
				vi++;
			}
	
			testMesh->estimateIndexCount(indices.size());
			std::vector<int>::const_iterator i = indices.begin();
			while(i != indices.end())
			{
				testMesh->index((*i));
				i++;
			}

			testMesh->end();
			
			std::string modelname = m_loggingDirectory;
			modelname += "/";
			modelname += name;
			modelname += ".ply";

			writePLY(modelname.c_str(), uniqueVerts, uniqueNormals, uniqueColors, indices, true);
			
			if(m_replayFile)
			{
				meshCount++;
				return;
			}
			
			Ogre::MeshPtr p = testMesh->convertToMesh(name);

			memset(name, 0, sizeof(name));
			std::string sName = m_loggingDirectory + "_sculptedNode";
			sprintf(name, "%s_%d", sName.c_str(), meshCount);

			Ogre::SceneNode* headNode = scene->getRootSceneNode()->createChildSceneNode();
			headNode->setPosition(0.f, 0.f, 0.f);
			headNode->setScale(m_cubeScale, m_cubeScale, m_cubeScale);
			headNode->attachObject(scene->createEntity(name, p));

			meshCount++;
		}
	}
}

Ogre::MovableObject * WorldBuilderScene::rayCastSelect(float & fDist)
{
	clearSelection();

	jvec3 vPos;
	getWandWorldSpace(vPos, true);
	//this correctly orients the direction of fire..
	jvec3 vWandDir;
	getWandDirWorldSpace(vWandDir, true);

	//perform a selection in the scene...TODO - test this..
	Ogre::RaySceneQuery *selectionRay = scene->createRayQuery(Ogre::Ray(Ogre::Vector3(vPos.x, vPos.y, vPos.z), Ogre::Vector3(vWandDir.x, vWandDir.y, vWandDir.z)));
	Ogre::RaySceneQueryResult & results = selectionRay->execute();
		
	//todo - figure out how to sort this and just grab the closest object..
	Ogre::RaySceneQueryResult::iterator it = results.begin();
	float fClosestDist = 9999999.f;
	int index = 0;
	Ogre::RaySceneQueryResultEntry *closest=0;
	for(it; it != results.end(); it++)
	{
		//all results in this list were selected by the ray.. grab closest selection..
		if(it->distance < fClosestDist)
		{
			fClosestDist = it->distance;
			closest = &(*it);
		}
	}

	if(closest != 0)
	{
		Ogre::MovableObject *obj = closest->movable;
		if(obj != 0)
		{
			//no selecting the floor for now..
			if(strcmp(obj->getName().c_str(), "Floor")!=0)
			{
				//printf("intersected entity named: %s\n", obj->getName().c_str());
				//obj->setVisible(false);
				if(obj->getParentSceneNode())
				{
					fDist = fClosestDist;
					obj->getParentSceneNode()->showBoundingBox(true);
					addSelection(obj);
					scene->destroyQuery(selectionRay);
					// Play a sound to let the user we've selected something.
					if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
						playSound("click");
					}
					return obj;
				}
			}
		}
	} else {
		// Whatever else happened, we didn't get a selection.
		// Play a sound giving the user a hint: they should point at an object.
		// Temporarily disabled for now: this fires after deselecting an object too, which is annoying.
		// if((fionaConf.appType==FionaConfig::HEADNODE) || (fionaConf.appType==FionaConfig::DEVLAB)  || (fionaConf.appType==FionaConfig::WINDOWED)) {
		// 	mSoundManager->getSound("select_nothing_pointed_at")->play();
		// }
	}

	return 0;
}

void WorldBuilderScene::saveScene(void)
{
	DotSceneLoader writer;
	std::string saveName = fionaConf.OgreMediaBasePath;
	saveName.append("levels\\");
	saveName.append(m_loggingDirectory);
	saveName.append("\\");
	
	//saveName.append(".scene");
	std::string meshDir = fionaConf.OgreMediaBasePath;
	meshDir.append("levels\\");
	meshDir.append(m_loggingDirectory);
	meshDir.append("\\");
	meshDir.append("mesh");

	if (!CreateDirectory (saveName.c_str(), NULL))
	{
		printf("couldn't create directory %s\n", saveName.c_str());
		exit(-1);
	}
	if (!CreateDirectory (meshDir.c_str(), NULL))
	{
		printf("couldn't create directory %s\n", meshDir.c_str());
		exit(-1);
	}

	//saveName.append(m_loggingDirectory.c_str());
	printf("creating directory %s\n", meshDir.c_str());
	writer.sceneExplore(scene, saveName.c_str(), meshDir.c_str(), m_loggingDirectory.c_str());
}

void WorldBuilderScene::writePLY(const char *fname, const std::vector<jvec3>& verts, const std::vector<jvec3>& norms, const std::vector<jvec3>& colors, const std::vector<int>& indices, bool binary) const
{
	 FILE *file = 0;
	 if(binary)
	 {
		 file = fopen(fname, "wb");
	 }
	 else
	 {
		file = fopen(fname, "w");
	 }

	 //I am assuming the indices are in order
	 int nfaces = indices.size() / 3;
 
	 fprintf(file, "ply\n");//
	 if (binary)
		 fprintf(file, "format binary_little_endian 1.0\n");
	 else
		 fprintf(file, "format ascii 1.0\n");// { ascii/binary, format version number }

	 fprintf(file, "element vertex %d\n", verts.size());// 
	 fprintf(file, "property float x\n");// { vertex contains float "x" coordinate }
	 fprintf(file, "property float y\n");// { y coordinate is also a vertex property }
	 fprintf(file, "property float z\n");// { z coordinate, too }
	 //normals
	 fprintf(file, "property float nx\n");// 
	 fprintf(file, "property float ny\n");// 
	 fprintf(file, "property float nz\n");// 
	 //colors
	 fprintf(file, "property uchar red\n");// 
	 fprintf(file, "property uchar green\n");// 
	 fprintf(file, "property uchar blue\n");// 
	 fprintf(file, "property uchar alpha\n");
 
	 fprintf(file, "element face %d\n", nfaces);// 
	 //note, I don't know what this next line does
	 fprintf(file, "property list uchar int vertex_indices\n");//
	 //{ L"vertex_indices" is a list of ints }
 
	 fprintf(file, "end_header\n");// { delimits the end of the header }
 
	 for (unsigned int i=0; i < verts.size(); i++)
	 {
		 if (binary)
		 {
			 //just to be safe I am doing these one by one
			 fwrite(&verts[i].x,sizeof(float), 1, file);
			 fwrite(&verts[i].y,sizeof(float), 1, file);
			 fwrite(&verts[i].z,sizeof(float), 1, file);
			 fwrite(&norms[i].x,sizeof(float), 1, file);
			 fwrite(&norms[i].y,sizeof(float), 1, file);
			 fwrite(&norms[i].z,sizeof(float), 1, file);

			 unsigned char ucolors[4];
			 ucolors[0] = (unsigned char)(colors[i].x * 255);
			 ucolors[1] = (unsigned char)(colors[i].y * 255);
			 ucolors[2] = (unsigned char)(colors[i].z * 255);
			 ucolors[3] = 255;
			 fwrite(&ucolors[0],sizeof(unsigned char), 4, file);
			 
		 }
		 else
			fprintf(file, "%f %f %f %f %f %f %d %d %d %d\n", verts[i].x, verts[i].y, verts[i].z, norms[i].x, norms[i].y, norms[i].z, (unsigned char)(colors[i].x * 255), (unsigned char)(colors[i].y * 255), (unsigned char)(colors[i].z * 255), 255);
	 }
	 for (unsigned int i=0; i < indices.size(); i+=3)
	 {
		  if (binary)
		  {
			   unsigned char unum = 3;
			    fwrite(&unum,sizeof(unsigned char), 1, file);
			   fwrite(&indices[i],sizeof(int), 1, file);
			    fwrite(&indices[i+1],sizeof(int), 1, file);
				 fwrite(&indices[i+2],sizeof(int), 1, file);
		  }
		  else
			 fprintf(file, "3 %d %d %d\n", indices[i], indices[i+1], indices[i+2]);
	 }
 
	 //i think that's it
	 fclose(file);

}

#ifndef LINUX_BUILD
void WorldBuilderScene::drawHand(const LeapData::HandData &hand)
{
	glLineWidth(5.f);
	glColor4f(1.f, 0.f, 1.f, 1.f);
	glBegin(GL_LINES);
	jvec3 vTracker;
	getTrackerWorldSpace(vTracker);
	vTracker.z -= 1.f;	//into screen 1 meter
	vTracker.y -= 0.5f;	//down 1 meter
	for(int i = 0; i < 5; ++i)
	{
		if(hand.fingers[i].valid)
		{
			float fLen = hand.fingers[i].length;
			glVertex3f(vTracker.x + hand.fingers[i].tipPosition[0], vTracker.y + hand.fingers[i].tipPosition[1], vTracker.z + hand.fingers[i].tipPosition[2]);
			glVertex3f(vTracker.x + (hand.fingers[i].tipPosition[0] + hand.fingers[i].tipDirection[0]*fLen), 
						vTracker.y + (hand.fingers[i].tipPosition[1] + hand.fingers[i].tipDirection[1]*fLen), 
						vTracker.z + (hand.fingers[i].tipPosition[2] + hand.fingers[i].tipDirection[2]*fLen));
		}
	}
	glEnd();
	glColor4f(1.f, 1.f, 1.f, 1.f);
	glLineWidth(1.f);
}
#endif
