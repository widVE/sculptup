#include "FionaOgre.h"


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

#include <Ogre/Physics/NxOgrePlaneGeometry.h>
#include <Ogre/Physics/NxOgreRemoteDebugger.h>

//ROSS TESTING
#include "OgreDotScene.h"

const Ogre::Vector3 FionaOgre::CAVE_CENTER = Ogre::Vector3(-5.25529762045281, 1.4478, 5.17094851606241);

Ogre::Matrix4 toOgreMat(const tran& p)
{
	return Ogre::Matrix4(p.a00,p.a01,p.a02,p.a03,
						 p.a10,p.a11,p.a12,p.a13,
						 p.a20,p.a21,p.a22,p.a23,
						 p.a30,p.a31,p.a32,p.a33);
}

FionaOgre::FionaOgre(void): FionaScene(), sharedContext(NULL), root(NULL), scene(NULL), 
	camera(0), headLight(0), renderTexture(0), vp(0), physicsWorld(0), physicsScene(0), barrelMesh(0), clothMesh(0), cloth(0), critterRender(0), lastTime(0.f), lastOgreMat(Ogre::Matrix4::IDENTITY)
{

}

FionaOgre::~FionaOgre()
{
	NxOgre::World::destroyWorld();
}

void FionaOgre::addResPath(const std::string& path)
{
	Ogre::ResourceGroupManager& rman=Ogre::ResourceGroupManager::getSingleton();
	rman.addResourceLocation(path, "FileSystem");
}


void FionaOgre::addResourcePaths(void)
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
	if(sceneName.length() > 0)
	{
		size_t s = sceneName.find_last_of('\\');
		if(s != -1)
		{
			std::string dir = sceneName.substr(0, s-1);
			//get directory from scene name and add sub-dirs
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir);
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\mesh"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\material"));
			addResPath(fionaConf.OgreMediaBasePath+std::string("\\") + dir + std::string("\\bitmap"));
		}

	}
#endif
}

void FionaOgre::buttons(int button, int state)
{
	if(button == 2 && state == 1)
	{
		//TESTING on machine: (or wand_model type)
		//vec3 vWandDir = camOri.rot(-ZAXIS);	//this appears gets the camera's orientation correctly
		//vec3 vPos = camOri.rot(camPos); //need to multiply position by the orientation to get that correct

		//TESTING in dev lab: (or wand_world type)
		
		//vPos = WAND POSITION
		vec3 vPos = camPos + wandPos;	//might need to orient the wandPos somehow still...?
		//this correctly orients the direction of fire..
		vec3 vWandDir = wandOri.rot(-ZAXIS);
		vWandDir = camOri.rot(vWandDir);
		//vWandDir at this point is WAND ORIENTATION

		NxOgre::Matrix44 matLoc(NxOgre::Vec3(vPos.x, vPos.y, vPos.z), NxOgre::Quat(camOri.w, camOri.x, camOri.y, camOri.z));
		makeBox(matLoc, NxOgre::Vec3(vWandDir.x, vWandDir.y, vWandDir.z)*10.f);
	}
	else if(button == 3 && state == 1)
	{
		//makeBarrel(NxOgre::Matrix44::IDENTITY, NxOgre::Vec3(0.f, 0.f, 2.5f));
	}
	else if(button == 0 && state == 1)
	{
		//perform a selection in the scene...
		vec3 vPos = camPos + wandPos;	//might need to orient the wandPos somehow still...?
		//this correctly orients the direction of fire..
		vec3 vWandDir = wandOri.rot(-ZAXIS);
		vWandDir = camOri.rot(vWandDir);

		Ogre::RaySceneQuery *selectionRay = scene->createRayQuery(Ogre::Ray(Ogre::Vector3(vPos.x, vPos.y, vPos.z), Ogre::Vector3(vWandDir.x, vWandDir.y, vWandDir.z)));
		Ogre::RaySceneQueryResult & results = selectionRay->execute();
		
		//todo - figure out how to sort this and just grab the closest object..
		Ogre::RaySceneQueryResult::iterator it = results.begin();
		float fClosestDist = 9999999.f;
		int index = 0;
		Ogre::RaySceneQueryResultEntry closest;
		for(it; it != results.end(); it++)
		{
			//all results in this list were selected by the ray.. grab closest selection..
			if(it->distance < fClosestDist)
			{
				fClosestDist = it->distance;
				closest = *it;
			}
		}

		Ogre::MovableObject *obj = closest.movable;


		scene->destroyQuery(selectionRay);
	}
}

void FionaOgre::initOgre(std::string scene)
{
	sceneName = scene;
#ifdef WIN32
#ifdef _DEBUG
	root = new Ogre::Root("Plugins_d.cfg");
#else
	root = new Ogre::Root("Plugins.cfg");
#endif
	std::string workDir="";
#endif
		
#ifdef __APPLE__
	std::string workDir=Ogre::macBundlePath() + "/Contents/Resources/";
	root=new Ogre::Root(Ogre::getOgrePluginCFGPath());
#endif
	Ogre::RenderSystem* _rs=root->getRenderSystemByName("OpenGL Rendering Subsystem");
	root->setRenderSystem(_rs);
	_rs->setConfigOption(Ogre::String("RTT Preferred Mode"), Ogre::String("FBO"));
	root->initialise(false);
	addResPath(workDir);
	addResPath(".");
	addResPath(fionaConf.OgreMediaBasePath);

	// User resource path
	addResourcePaths();
}

void FionaOgre::initWin(WIN nwin, CTX ctx, int gwin, int w, int h)
{
	Ogre::NameValuePairList misc;
#ifdef WIN32
	HWND hWnd = (HWND)nwin;
	HGLRC hRC = (HGLRC)ctx;
	misc["externalWindowHandle"] = Ogre::StringConverter::toString((int)hWnd);
	misc["externalGLContext"] = Ogre::StringConverter::toString((unsigned long)hRC);
#elif defined __APPLE__
	NSView* view = (NSView *)(nwin);
	NSOpenGLContext* context = (NSOpenGLContext *)(ctx);
	misc["macAPI"] = "cocoa";
	misc["macAPICocoaUseNSView"] = "true";
	misc["externalWindowHandle"] = Ogre::StringConverter::toString((size_t)view);
	misc["externalGLContext"] = Ogre::StringConverter::toString((unsigned long)context);
#endif
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
		camera->setPosition(Ogre::Vector3(0,10.0,0));
			
		headLight=scene->createLight("OgreGLUTDefaultHeadLight");
		headLight->setType(Ogre::Light::LT_POINT);
		headLight->setDiffuseColour(1, 1, 1);
		headLight->setPosition(Ogre::Vector3(1,2,2));
//			headLight->setDirection(vec3(0,-1,-2));
	}
	vp = win->addViewport(camera);
	vp->setDimensions(.0f, .0f, 1.0f, 1.0f);
		
	// Add to management system
	wins.push_back(FionaOgreWinInfo(win,nwin,gwin,ctx,vp));
	if(firstWindow) 
	{
		setupScene(scene);
	}
}

void FionaOgre::render(void)
{
	FionaScene::render();
	WIN win = FionaUTGetNativeWindow();
	if( !isInited(win) )
	{
		CTX ctx = FionaUTGetContext(); 
		initWin(win,ctx,glutGetWindow(),
				glutGet(GLUT_WINDOW_WIDTH),glutGet(GLUT_WINDOW_HEIGHT));
	}

	vp->setBackgroundColour(Ogre::ColourValue(fionaConf.backgroundColor.x,
												fionaConf.backgroundColor.y,
												fionaConf.backgroundColor.z));
	if( _FionaUTIsInFBO() )
	{
		int i = findWin(glutGetWindow());
#ifdef __APPLE__
		wins[i].owin->resize(_FionaUTGetFBOWidth(),_FionaUTGetFBOHeight());
#else
//			wins[i].vp->setDimensions(0,0,_FionaUTGetFBOWidth()/(float)glutGet(GLUT_WINDOW_WIDTH),
//				_FionaUTGetFBOHeight()/(float)glutGet(GLUT_WINDOW_HEIGHT));
#endif
	}
	else
	{
		//this is screwing things up.. ogre 1.8..
		int wid = glutGet(GLUT_WINDOW_WIDTH);
		int hei = glutGet(GLUT_WINDOW_HEIGHT);
		//printf("%d, %d\n", wid, hei);
		resize(glutGetWindow(),wid,hei);
	}

	tran m, p;
	glGetFloatv(GL_MODELVIEW_MATRIX, m.p);
	glGetFloatv(GL_PROJECTION_MATRIX, p.p);
	lastOgreMat = toOgreMat(m);
	camera->setCustomViewMatrix(TRUE, lastOgreMat);
	camera->setCustomProjectionMatrix(TRUE, toOgreMat(p));
	if( FionaIsFirstOfCycle() )
	{
		/*float currTime = FionaUTTime();
		if(lastTime == 0.f)
		{
			lastTime=currTime;
		}
		physicsWorld->advance(currTime-lastTime);
		lastTime = currTime;*/
		if(physicsWorld)
		{
			physicsWorld->advance(fionaConf.physicsStep);
		}
	}
	
	root->renderOneFrame();
}

void FionaOgre::resize(int gwin, int w, int h)
{
	int i = findWin(gwin); if(i<0 ) return;
	wins[i].owin->resize(w,h);
#ifdef WIN32
	wins[i].owin->windowMovedOrResized ();
#endif
}

void FionaOgre::setupScene(Ogre::SceneManager* scene)
{
	//THIS NEEDS TO BE CALLED BEFORE BEING ABLE TO LOAD RESOURCES
	Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
	
	//ROSS TESTING
	if(!sceneName.empty())
	{
		//initialize default physics settings..
		physicsWorld = NxOgre::World::createWorld();
		physicsWorld->getRemoteDebugger()->connect();

		NxOgre::SceneDescription scene_description;
		scene_description.mGravity = NxOgre::Constants::MEAN_EARTH_GRAVITY;
		scene_description.mUseHardware = false;
  
		physicsScene = physicsWorld->createScene(scene_description);
		physicsScene->getMaterial(0)->setAll(0.1,0.9,0.5);
		NxOgre::PlaneGeometryDescription planeDesc(NxOgre::Vec3(0,1,0), -1.4478);
		physicsScene->createSceneGeometry(planeDesc);

		critterRender = new Critter::RenderSystem(physicsScene, scene);

		NxOgre::ResourceSystem::getSingleton()->openProtocol(new Critter::OgreResourceProtocol());
  
		DotSceneLoader loader;
		loader.setCritterRender(critterRender);

		loader.parseDotScene(fionaConf.OgreMediaBasePath+sceneName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, scene);

		//position the user in the center of the CAVE...!
		camPos.set(CAVE_CENTER.x, CAVE_CENTER.y, CAVE_CENTER.z);

		//below two lines will turn on physics debugging
		critterRender->setVisualisationMode(NxOgre::Enums::VisualDebugger_ShowAll);
		physicsWorld->getVisualDebugger()->enable();
	}
	else
	{
		/*if(fionaConf.appType == 2)//APP_OGRE_HEAD)
		{
			Ogre::SceneNode* headNode = scene->getRootSceneNode()->createChildSceneNode();
			headNode->setScale(0.02f, 0.02f, 0.02f);
			headNode->attachObject(scene->createEntity("Head", "ogrehead.mesh"));
		}
		else
		{*/
			//OGRE OpenAL Sound Example!
			/*OgreOggSound::OgreOggSoundManager *mSoundManager = OgreOggSound::OgreOggSoundManager::getSingletonPtr();

			if (mSoundManager->init())
			{
				// Create a streamed sound, no looping, no prebuffering
				if ( mSoundManager->createSound("Sound1", fionaConf.OgreMediaBasePath+std::string("Box\\hand.wav"), false, false, false) )
				{
					mSoundManager->getSound("Sound1")->play();
				}
			}*/

			//scene->setShadowTechnique(Ogre::SHADOWTYPE_STENCIL_ADDITIVE);
			/*scene->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
			scene->setShadowColour(Ogre::ColourValue(0.5, 0.5, 0.5));
			scene->setShadowTextureSize(1024);
			scene->setShadowTextureCount(1);*/

			//load up default plane, etc..
			Ogre::ColourValue background = Ogre::ColourValue(fionaConf.backgroundColor.x, fionaConf.backgroundColor.y, fionaConf.backgroundColor.z);
			scene->setFog(Ogre::FOG_EXP, background, 0.001, 800, 1000);
	
			//scene->setSkyBox(true, "Examples/CloudyNoonSkyBox");  // set a skybox

			Ogre::MeshManager::getSingleton().createPlane("floor", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
				Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.f), 1000, 1000, 1,1 , true, 1, 1, 1, Ogre::Vector3::UNIT_Z);

			Ogre::Entity* floor = scene->createEntity("Floor", "floor");
			floor->setMaterialName("ground-from-nxogre.org");
			floor->setCastShadows(false);
			Ogre::SceneNode* planeNode = scene->getRootSceneNode()->createChildSceneNode();
			planeNode->setPosition(0.0, -1.4478, 0.0);
			planeNode->attachObject(floor);
			//scene->getRootSceneNode()->attachObject(floor);

			scene->setAmbientLight(Ogre::ColourValue(0.3, 0.3, 0.3));

			Ogre::Light* light = scene->createLight();
			light->setType(Ogre::Light::LT_POINT);
			light->setPosition(-10, 40, 20);
			light->setSpecularColour(Ogre::ColourValue::White);

			physicsWorld = NxOgre::World::createWorld();
			physicsWorld->getRemoteDebugger()->connect();

			NxOgre::SceneDescription scene_description;
			scene_description.mGravity = NxOgre::Constants::MEAN_EARTH_GRAVITY;
			scene_description.mUseHardware = false;
  
			physicsScene = physicsWorld->createScene(scene_description);
			physicsScene->getMaterial(0)->setAll(0.1,0.9,0.5);
			NxOgre::PlaneGeometryDescription planeDesc(NxOgre::Vec3(0,1,0), -1.4478);
			physicsScene->createSceneGeometry(planeDesc);

			critterRender = new Critter::RenderSystem(physicsScene, scene);
			makeBox(NxOgre::Matrix44(NxOgre::Vec3(0.0, 4.0, 0.0)), NxOgre::Vec3(0.0, 1.0, 0.0));
			makeBox(NxOgre::Matrix44(NxOgre::Vec3(2.0, 4.0, 0.0)), NxOgre::Vec3(0.0, 1.0, 0.0));
			makeBox(NxOgre::Matrix44(NxOgre::Vec3(-2.0, 4.0, 0.0)), NxOgre::Vec3(0.0, 1.0, 0.0));

			makeCloth(NxOgre::Vec3(0,6,0));

			camPos.set(CAVE_CENTER.x, CAVE_CENTER.y, CAVE_CENTER.z);
		//}
	}
}

Critter::Body* FionaOgre::makeBox(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3 & initialVelocity)
{
	Critter::BodyDescription bodyDescription;
	bodyDescription.mMass = 40.0f;
	bodyDescription.mLinearVelocity = initialVelocity;
  
	Critter::Body* box = critterRender->createBody(NxOgre::BoxDescription(1,1,1), globalPose, "cube.1m.mesh", bodyDescription);

	return box;
}

Critter::Body* FionaOgre::makeBarrel(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3& initialVelocity)
{
	if (barrelMesh == 0)
	{
		barrelMesh = NxOgre::MeshManager::getSingleton()->load("ogre://barrel.nxs", "barrel");
		//barrelMesh = NxOgre::MeshManager::getSingleton()->load("barrel.nxs", "barrel");
	}
  
	Critter::BodyDescription bodyDescription;
	bodyDescription.mMass = 40.0f;
	bodyDescription.mLinearVelocity = initialVelocity;
  
	Critter::Body* barrel = critterRender->createBody( NxOgre::ConvexDescription(barrelMesh), globalPose, "barrel.mesh", bodyDescription);
  
	return barrel;
}


void FionaOgre::makeCloth(const NxOgre::Vec3& barPosition)
{
	NxOgre::Vec3 pos = barPosition;
	NxOgre::Vec2 clothSize(8,4);

	pos.x -= clothSize.x * 0.5f;
  
	if (clothMesh == 0)
		clothMesh = NxOgre::MeshGenerator::makePlane(clothSize, 0.1, NxOgre::Enums::MeshType_Cloth, "file://rug.xcl");
  
	NxOgre::Vec3 holderPos = pos;
	holderPos.x += clothSize.x * 0.5f;;
	holderPos.y += 0.05f;
	holderPos.z -= 0.05f;
	NxOgre::SceneGeometry* geom = physicsScene->createSceneGeometry(NxOgre::BoxDescription(10, 0.1, 0.1), holderPos);
  
	Ogre::SceneNode* geom_node = critterRender->createSceneNodeEntityPair("cube.mesh", holderPos);
	geom_node->scale(0.1, 0.001, 0.001);
  
	NxOgre::ClothDescription desc;
	desc.mMesh = clothMesh;
	desc.mThickness = 0.2;
	desc.mFriction = 0.5;
	//desc.mFlags |= NxOgre::Enums::ClothFlags_BendingResistance;
	//desc.mFlags |= NxOgre::Enums::ClothFlags_TwoWayCollisions;
  
	desc.mGlobalPose.set(pos);
  
	cloth = critterRender->createCloth(desc, "wales");
	cloth->attachToShape((*geom->getShapes().begin()), NxOgre::Enums::ClothAttachmentFlags_Twoway);
}