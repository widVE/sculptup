//
//  FionaOgre.h
//  FionaUT
//
//  Created by Hyun Joon Shin on 5/10/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef _FIONA_OGRE_H__
#define _FIONA_OGRE_H__

#ifdef __APPLE__
//#include <GLEW/glew.h>
#include <OpenGL/GL.h>
#elif defined WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <GL/glew.h>
#endif

#include <Windows.h>
#include <vector>

#if defined __JMATH_H__ && !defined JMATH_OGRE_WRAPPER_
#error JMATH should be included after Ogre. \
If you included "FionaUT" before "FionaOgre", \
Swap them or do not include either FionaUT or Kit3D.
#endif

#include "FionaUT.h"
#include "FionaScene.h"

#ifdef __APPLE__
#include <Ogre/OSX/macUtils.h>
#include <Cocoa/Cocoa.h>
#endif

#include <Ogre/OgrePrerequisites.h>

#include <Ogre/Sound/OgreOggSoundManager.h>

#include <Ogre/Physics/NxOgreWorld.h>
#include <Ogre/Physics/NxOgreScene.h>

#include "critter/critter.h"

struct FionaOgreWinInfo
{
	Ogre::RenderWindow*		owin;
	WIN						nwin;
	CTX						ctx;
	Ogre::Viewport*			vp;
	int						gwin;
	FionaOgreWinInfo(Ogre::RenderWindow* ow, WIN nw, int gw, CTX c, Ogre::Viewport* v)
	: owin(ow), nwin(nw), ctx(c), vp(v), gwin(gw){}
};

class FionaOgre : public FionaScene
{
public:
	FionaOgre(void);
	virtual ~FionaOgre(void);

	void initOgre(std::string scene=std::string(""));

	void resize(int gwin, int w, int h);

	inline Ogre::SceneManager* getScene(void) { return scene; }
	inline Ogre::Light* getHeadlight(void) { return headLight; }
	inline Ogre::Camera* getCamera(void) { return camera; }

	// Following function are to be overrod for applications.
	virtual void render(void);
	virtual void setupScene(Ogre::SceneManager* scene);
	virtual void addResourcePaths(void);
	virtual void buttons(int button, int state);

protected:
	inline bool isInited(WIN win) { return findWin(win)>=0; }

private:
	std::vector<FionaOgreWinInfo>	wins;

	inline int findWin(WIN win)
	{ for(int i=0;i<(int)wins.size();i++) if(wins[i].nwin==win)return i; return -1;}
	inline int findWin(int gwin)
	{ for(int i=0;i<(int)wins.size();i++) if(wins[i].gwin==gwin)return i; return -1;}

	CTX						sharedContext;
	Ogre::Root*				root;
	Ogre::SceneManager*		scene;
	Ogre::Camera*			camera;
	Ogre::Light*			headLight;
	Ogre::RenderTexture*	renderTexture;
	Ogre::Viewport*			vp;
	NxOgre::World*			physicsWorld;
	NxOgre::Scene*			physicsScene;
	NxOgre::Mesh*			barrelMesh;
	NxOgre::Mesh*			clothMesh;
	NxOgre::Cloth*			cloth;
	Critter::RenderSystem*  critterRender;

	float					lastTime;
	Ogre::Matrix4			lastOgreMat;

	std::string				sceneName;
	
	const static Ogre::Vector3 CAVE_CENTER;

	void		initWin(WIN nwin, CTX ctx, int gwin, int w, int h);

	Critter::Body * makeBox(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3 & initialVelocity = NxOgre::Vec3::ZERO);
	Critter::Body * makeBarrel(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3& initialVelocity);
	void			makeCloth(const NxOgre::Vec3& barPosition);

protected:
	virtual void addResPath(const std::string& path);
};

#endif