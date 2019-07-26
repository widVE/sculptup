#ifndef _WORLD_BUILDER_SCENE_H__
#define _WORLD_BUILDER_SCENE_H__

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <GL/glew.h>
#endif

#include <Windows.h>
#include <vector>

/*#if defined __JMATH_H__ && !defined JMATH_OGRE_WRAPPER_
#error JMATH should be included after Ogre. \
If you included "FionaUT" before "FionaOgre", \
Swap them or do not include either FionaUT or Kit3D.
#endif*/

#include "FionaUT.h"
#include "FionaScene.h"
#include "Cg/cg.h"

#include <Ogre/OgrePrerequisites.h>

#include <Ogre/Sound/OgreOggSoundManager.h>

#include <Ogre/Physics/NxOgreWorld.h>
#include <Ogre/Physics/NxOgreScene.h>

#include "critter/critter.h"

//class VolumeRender;
class WorldBuilderSystem;

struct WorldBuilderSceneWinInfo
{
	Ogre::RenderWindow*		owin;
	WIN						nwin;
	CTX						ctx;
	Ogre::Viewport*			vp;
	int						gwin;
	WorldBuilderSceneWinInfo(Ogre::RenderWindow* ow, WIN nw, int gw, CTX c, Ogre::Viewport* v)
	: owin(ow), nwin(nw), ctx(c), vp(v), gwin(gw){}
};

class Ogre::MovableObject;

class WorldBuilderScene : public FionaScene
{
public:
	class GridCell
	{
	public:
		float val[8];
		float redVal[8];
		float greenVal[8];
		float blueVal[8];
		jvec3 pos[8];
	};

	enum {
		SCULPTER=0,
		ERASER=1,
		PAINT=2,
		NUM_TOOLS
	} SculptTools;

	enum {
		BLOB=0,
		CUBE=1,
		NUM_ADD_MODES
	} AddMode;

	enum {
		TRANSLATE=0,
		ROTATE=1,
		SCALE=2,
		NUM_TRANSFORM_MODES
	}TransformMode;

	WorldBuilderScene();
	virtual ~WorldBuilderScene();
	
	void initOgre(std::string scene=std::string(""));

	void resize(int gwin, int w, int h);

	inline Ogre::SceneManager* getScene(void) { return scene; }
	inline Ogre::Light* getHeadlight(void) { return headLight; }
	inline Ogre::Camera* getCamera(void) { return camera; }
	
	//world mode stuff in-progress..
	void					addSelection(Ogre::MovableObject *obj) { m_currentSelection.push_back(obj); }
	void					clearSelection(void);
	void					changeAddMode(void);
	void					changeModes(bool bWorldMode);
	void					changeTool(void);
	void					changeTSR(void);
	unsigned int			getAddMode(void) const { return m_addMode; }
	const jvec3 &			getAddCube(void) const { return m_addCube; }
	float					getBeamRadius(void) const { return m_beamRadius; }
	float					getBeamLength(void) const { return m_beamLength; }
	float					getCubeScale(void) const { return m_cubeScale; }
	const jvec3 &			getCurrentColor(void) const { return m_currColor; }
	unsigned int			getCurrentTool(void) const { return m_currentTool; }
	float					getFlowMultiplier(void) const { return m_flowMultiplier; }
	const std::string &		getLoggingDirectory(void) const { return m_loggingDirectory; }
	unsigned int			getTSR(void) const { return m_tsr; }
	void					getSecondTrackerPos(jvec3 &vOut, const jvec3 &vOffset=jvec3(0.f, 0.f, 0.f)) const;
	WorldBuilderSystem *	getSystem(void) { return m_wbSys; }
	float					getWiiFitRotation(void) const { return m_wiiFitRotation; }
	bool					isWorldMode(void) const { return m_worldMode; }
	int						numSelected(void) const { return m_currentSelection.size(); }
	Ogre::MovableObject *	getSelected(int index) { return m_currentSelection[index]; }
	Ogre::MovableObject *	rayCastSelect(float & fDist);
	void					saveScene(void);
	void					setAddCube(const jvec3 &vAddCube) { m_addCube = vAddCube; }
	void					setBeamRadius(float beamRadius) { m_beamRadius = beamRadius; }
	void					setBeamLength(float beamLength) { m_beamLength = beamLength; }
	void					setFlowMultiplier(float flow) { m_flowMultiplier = flow; }
	void					setTool(unsigned int tool);
	void					setCubeScale(float scale) { m_cubeScale = scale; }
	

	//over-rides from FionaScene
	virtual void			addResourcePaths(void);
	virtual void			buttons(int button, int state);
	virtual void			keyboard(unsigned int key, int x, int y);
	virtual void			onExit(void);
	virtual void			render(void);
	virtual void			setupScene(Ogre::SceneManager* scene);
	virtual void			updateLeap(void);

	static bool		inBounds(const jvec3& pos);
	
	//helper functions for grabbing tracked location
	jvec3			getWandPos(float x_off = 0.0, float y_off = 0.0, float z_off = 0.0) const;
	jvec3			getNormalizedPos(const jvec3 & pos) const;
	jvec3			getNormalizedWandPos(float x_off = 0.0, float y_off = 0.0, float z_off = 0.0) const;
	void			getNormalizedSecondTrackerPos(jvec3 &vOut, const jvec3 &vOffset=jvec3(0.f, 0.f, 0.f)) const;

	static const float EMG_MULTIPLIER;
	static const float WAND_OFFSET_DISTANCE;
	static const float MINIMUM_FLOW_SIZE;
	
	void			playSound(const char *name);

	//sound stuff
	OgreOggSound::OgreOggSoundManager *mSoundManager;

	//LeapData		leap;

protected:
	inline bool isInited(WIN win) { return findWin(win)>=0; }
	void addResPath(const std::string& path);

private:
	std::vector<WorldBuilderSceneWinInfo>	wins;

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
	Critter::RenderSystem*  critterRender;	//for physics

	float					lastTime;
	Ogre::Matrix4			lastOgreMat;

	std::string				sceneName;		//for loading a specific scene..
	
	std::vector<Ogre::MovableObject*> m_currentSelection;	//currently selected items in world mode

	const static Ogre::Vector3 CAVE_CENTER;

	//private functions
	void			initWin(WIN nwin, CTX ctx, int gwin, int w, int h);
	Critter::Body * makeBox(const NxOgre::Matrix44& globalPose, const NxOgre::Vec3 & initialVelocity = NxOgre::Vec3::ZERO);

	//main init function
	void			initMarchingCube();
	void			initShader(GLuint programObject, const char *filen, GLuint type);
	
	//main rendering function...
	void			renderMarchingCubes(void);
	void			swizzledWalk(int &n, float *gridData, vec3i pos, vec3i size, const jvec3 &cubeSize);
	
	void			exportPlyScene(void);
	void			handleLogging(void);

	static int triTable[256][16];
	static int edgeTable[256];

	static const float WII_FIT_ROTATION_SPEED;
	
	//general flags
	bool m_worldMode;
	bool wireframe;
	bool initialized;
	bool enableVBO;
	bool enableSwizzledWalk;
	bool m_cubeWireFrame;
	bool m_raymarchRender;
	//added by KEvin
	bool enableDisplayList;
	GLuint m_DisplayList;

	unsigned int m_addMode;
	unsigned int m_tsr;
	unsigned int m_currentTool;

	//GLSL
	GLuint programObject;

	GLuint edgeTableTex;
	GLuint triTableTex;

	GLuint gridDataVAO;
	GLuint gridDataBuffId;
	GLuint gridDataSwizzledBuffId;

	vec3i dataSize;

//	float *dataField[NUM_VOLUME_TEXTURES];
	float *gridData;

	CGcontext cgContext;
	WorldBuilderSystem *m_wbSys;
	//FluidBuilderSystem *m_wbSys;
	
	//software marching cubes to ogre mesh creation functions!
	void	tesselateCurrentVolume(float *data, float *redData, float *greenData, float *blueData, vec3i size, vec3i gridsize, float isolevel, std::vector<jvec3> & outVerts, std::vector<jvec3> & outColors, std::vector<jvec3> & outNormals);
	void	makeMesh(float *geomData, float *redData, float *greenData, float *blueData);
	int		polygonise(GridCell &grid, float isolevel, jvec3 *triangles, jvec3 *colors);
	jvec3	vertexInterp(float isolevel, jvec3 p1, jvec3 p2, float valp1, float valp2, float & mu);
	void	writePLY(const char *fname, const std::vector<jvec3> & verts, const std::vector<jvec3> & norms, const std::vector<jvec3> & colors, const std::vector<int> & indices, bool binary=true) const;

	void	drawHand(const LeapData::HandData &hand);

	//VolumeRender *m_wbRenderer;

	float m_wiiFitRotation;
	float m_beamLength;
	float m_beamRadius;
	float isolevel;
	float m_cubeScale;
	float m_flowMultiplier;

	jvec3 cubeSize;
	jvec3 cubeStep;
	jvec3 m_currColor;
	jvec3 m_addCube;

	//for data logging
	void makeLogDirectory();
	std::string m_loggingDirectory;
	FILE * m_wandlogfile;
	FILE * m_replayFile;
	bool m_replayDumpPLY;
};

#endif