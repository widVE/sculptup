/**** Geometry Shader Marching Cubes Example
	* Copyright Cyril Crassin, January 2007.
	* This code is partially based on the example of 
	* Paul Bourk "Polygonising a scalar field" located at :
	* http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonise/
****/

#ifndef WORLDBUILDERG80_H 
#define WORLDBUILDERG80_H 

#include "GLAppTestG80.h"
#include <Cg/cg.h>
#include "WorldBuilderSystem.h"

using namespace NemoGraphics;

class WorldBuilderG80: public GLAppTestG80 {

	//Mouse control
	Vector2i oldMousePos;
	bool mouseButtons[3];

	Vector3f sphereLightPos;
	Vector3f viewOrient;


	//general flags
	bool pause;
	bool wireframe;

	//GLSL
	GLhandleARB programObject;
	GLhandleARB programObjectGS;
	GLhandleARB programObjectFS;

	GLuint edgeTableTex;
	GLuint triTableTex;

	GLuint dataFieldTex[3];

	Vector3f cubeSize;
	Vector3f cubeStep;

	Vector3i dataSize;

	float *dataField[3];
	float isolevel;

	bool animate;
	bool autoWay;
	bool enableVBO;
	bool enableSwizzledWalk;

	int mode;

	int curData;

	float *gridData;
	GLuint gridDataBuffId;
	GLuint gridDataSwizzledBuffId;

	// volume manipulation
	CGcontext cgContext;
	WorldBuilderSystem *wbSys;

protected:
	virtual void keyboard(unsigned char key);
	virtual void mouse(int button, int state, int x, int y);
	virtual void mouseMove(int x, int y);
	virtual void mousePassiveMove(int x, int y);

	virtual void reshape(int w, int h);

	virtual void idle() ;
	virtual void display() ;

	virtual void init();

	void perspectiveGL( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar );

	void initShader(GLhandleARB programObject, const char *file, GLuint type);
public:

	WorldBuilderG80(const char *title, int w, int h, int flags = 0);
	WorldBuilderG80(int *argcp, char **argv, const char *title, int w, int h, int flags = 0);

};

#endif