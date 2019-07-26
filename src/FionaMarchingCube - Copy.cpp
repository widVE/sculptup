
#include "FionaMarchingCube.h"

#include <GL/glew.h>
#include <time.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <fstream>

#include "WorldBuilderSystem.h"
#include "VolumeBuffer.h"

using std::cout;
using std::endl;

const float FionaMarchingCube::WII_FIT_ROTATION_SPEED = 0.5f;

FionaMarchingCube::FionaMarchingCube(void) 
	: FionaScene(), pause(false), wireframe(false), initialized(false), animate(false), autoWay(true), enableVBO(false), enableSwizzledWalk(true), 
	curData(0), mode(1), isolevel(0.5f), m_volumedata(0), gridData(0), m_wbSys(0), m_cubeScale(0.5f), m_cubeWireFrame(true)
	,m_beamPoint(vec3()), m_beamLength(0.1f), m_beamRadius(0.05), m_wiiFitRotation(0.f)
{
  float cube = 64.f;
  int data = 128;
	cubeSize=vec3(cube,cube,cube);
	cubeStep=vec3(2.0f / cubeSize.x, 2.0f / cubeSize.y, 2.0f / cubeSize.z);
	dataSize=vec3i(data,data,data);
}

FionaMarchingCube::~FionaMarchingCube()
{
	if(m_volumedata)
	{
		delete [] m_volumedata;
	}

	if(gridData)
	{
		delete [] gridData;
	}
}

void FionaMarchingCube::getNormalizedPos(vec3 &vPos) const
{
	float toDeg = m_wiiFitRotation * PI/180;
	mat3 m(cosf(toDeg), 0.f, -sinf(toDeg), 0.f, 1.f, 0.f, sinf(toDeg), 0.f, cosf(toDeg));
	vPos = m.transpose().inv()  * vPos;

	vec3 pos = vec3(m_cubeScale, m_cubeScale, m_cubeScale);
    pos.x += vPos.x; pos.y += vPos.y; pos.z += vPos.z;
    pos /= (m_cubeScale*2);
}

vec3 FionaMarchingCube::getNormalizedWandPos() const 
{
	vec3 vPos;
	getWandWorldSpace(vPos);
	getNormalizedPos(vPos);

	///quat angleAxis(m_wiiFitRotation, 0.f, 1.f, 0.f);
	//vec3 vInvPos = angleAxis.inv().rot(vPos);
	//below example is how to get the wand's direction.
	//could use vWandDir here to offset the blob to appear away from the tip of the wand if we wanted...
	//vec3 vWandDir;
	//getWandDirWorldSpace(vWandDir, false);
  
	return vPos;
}

static bool inBounds(const vec3& pos) {
    if (pos.x >= 0.0f && pos.y >= 0.0f && pos.z >= 0.0f &&
    pos.x <= 1.0f && pos.y <= 1.0f && pos.z <= 1.0f) {
        return true;
    } return false;
}

void FionaMarchingCube::addBlob() 
{
	if(buttonDown(0)) 
	{
		vec3 pos = this->getNormalizedWandPos();
		vec3 vWandDir;
		getWandDirWorldSpace(vWandDir, true);
		vec3 wp = (m_beamLength * vWandDir);
		getNormalizedPos(wp);
		//vec3 vWandDirY;
		// getWandDirWorldSpace(vWandDirY, false);

		vec3 wpos = pos+wp;//(wp /(m_cubeScale*2));
		if (inBounds(pos)) 
		{
			if(buttonDown(5)) 
			{
				//m_wbSys->add(pos.x, pos.y, pos.z, 0.2f, -1.0f*m_cubeScale);		//make it so this erase value matches the cube indicator..
				m_wbSys->erase(pos.x, pos.y, pos.z, wpos.x, wpos.y, wpos.z, 0.2f, m_cubeScale);
			} 
			else 
			{
				m_wbSys->add(pos.x, pos.y, pos.z, 0.2f, m_cubeScale);
			}
		}
		cout << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z << endl;
	}
}

void FionaMarchingCube::drawSecondTracker(void)
{
	glPushMatrix();
	glDisable(GL_LIGHTING);
	glColor3f(1.0f, 0.0f, 1.0f);
	vec3 vSecondTrackerPos;
	getSecondTrackerWorldSpace(vSecondTrackerPos);
	//printf("Second tracker: %f, %f, %f\n", vSecondTrackerPos.x, vSecondTrackerPos.y, vSecondTrackerPos.z);
	glTranslatef(vSecondTrackerPos.x, vSecondTrackerPos.y, vSecondTrackerPos.z);
	glutWireCube(0.05);
	glEnable(GL_LIGHTING);
	glPopMatrix();
}

void FionaMarchingCube::drawIndicator() {
    // if holding trigger, show erase shape
    if(buttonDown(5)) {

        vec3 pos;
		getWandWorldSpace(pos);

		//vec3 pos = this->getNormalizedWandPos();
		vec3 vWandDir;
		getWandDirWorldSpace(vWandDir, true);
		
        glColor3f(1.0f, 1.0f, 1.0f);
		m_beamPoint = pos + (m_beamLength * vWandDir);
		vec3 zero = vec3(0.0,0.0,0.0);
		//vec3 vWandDirY;
		// getWandDirWorldSpace(vWandDirY, false);
		quat q(vWandDir);
		
		glPushMatrix();
        glDisable(GL_LIGHTING);
		
		/*
		glRotate(q);
		//glScalef(m_beamRadius, m_beamRadius, m_beamLength);
		glTranslatef(pos.x, pos.y, pos.z);

		//glTranslatef(0.5, 0.5, 0.5);
		
		//glTranslatef(-0.5, -0.5, -0.5);

		
		
        glutWireCube(1.0);
		*/
		glLineWidth(10);
		glLine(pos, m_beamPoint);
		glLineWidth(1);
        glEnable(GL_LIGHTING);
        glPopMatrix();
	

        return;
    }
    // if holding nothing, show draw shape
    if(!fionaConf.currentButtons) {
        vec3 pos;
		getWandWorldSpace(pos);
        glPushMatrix();
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 1.0f, 1.0f);
        glTranslatef(pos.x, pos.y, pos.z);
        glutWireSphere(0.03, 6, 6);
        glEnable(GL_LIGHTING);
        glPopMatrix();
        return;
    }
}

void FionaMarchingCube::buttons(int button, int state)
{
  if(button == 0 && state == 1)
  {
		/*vec3 vPos = camPos + wandPos;	//might need to orient the wandPos somehow still...?
		//this correctly orients the direction of fire..
		vec3 vWandDir = wandOri.rot(YAXIS);
		vWandDir = camOri.rot(vWandDir);
    vec3 pos = vec3(0.5, 0.5, 0.5);
    pos.x += vPos.x; pos.y += vPos.y; pos.z += vPos.z;
    if (pos.x >= 0.0f && pos.y >= 0.0f && pos.z >= 0.0f &&
    pos.x <= 1.0f && pos.y <= 1.0f && pos.z <= 1.0f) {
        m_wbSys->add(pos.x, pos.y, pos.z);
    }
    cout << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z << endl;*/
		//vWandDir at this point is WAND ORIENTATION
		//mode = 0;
		// float x = rand()/(float)RAND_MAX, 
			// y = rand()/(float)RAND_MAX,
			// z = rand()/(float)RAND_MAX;
		// m_wbSys->add(x,y,z);
	}
	else if(button == 1 && state == 1)
	{
		m_wbSys->reset();
		//mode = 1;
	}
	else if(button == 2 && state == 1)
	{
		// mode = 2;
		animate = !animate;
	}
	else if(button == 3 && state == 1)
	{
		this->wireframe = !this->wireframe;
		//curData=(curData+1)%3;
		//glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_3D, dataFieldTex[curData]);
		//glBindTexture(GL_TEXTURE_3D, m_wbSys->getStateBuffer()->getTexture());
		// m_wbSys->getStateBuffer()->setTexID(this->dataFieldTex[curData]);
	}
	else if(button == 4 && state == 1)
	{
		//make this button complete our model and turn it into an ogre mesh?

		//wireframe = !wireframe;
		//m_cubeWireFrame = !m_cubeWireFrame;
	}
	// else if(button == 5 && state == 1)
	// {
	// }
}

void FionaMarchingCube::init(void)
{
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

  int size = dataSize.x * dataSize.y * dataSize.z;
	m_volumedata = new GLfloat[size];
  memset(m_volumedata, 0.0f, size);

	//Print Avalaible OpenGL extensions
	std::cout<<glGetString(GL_EXTENSIONS)<<"\n";

//Disable Vsync on windows plateform
#ifdef WIN32
	//wglSwapIntervalEXT(0);	//FIONAUT - turned this off as we need to vsync in cave..
#endif

	srand(time(0));


	/////GLSL/////

	//Program object creation
	programObject = glCreateProgramObjectARB();


	////Shaders loading////
	//Geometry Shader loading
	initShader(programObject, "Shaders/TestG80_GS2.glsl", GL_GEOMETRY_SHADER_EXT);
	//Geometry Shader require a Vertex Shader to be used
	initShader(programObject, "Shaders/TestG80_VS.glsl", GL_VERTEX_SHADER_ARB);
	//Fragment Shader for per-fragment lighting
	initShader(programObject, "Shaders/TestG80_FS.glsl", GL_FRAGMENT_SHADER_ARB);
	////////


	//Get max number of geometry shader output vertices
	GLint temp;
	glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_VERTICES_EXT,&temp);
	std::cout<<"Max GS output vertices:"<<temp<<"\n";
	
	////Setup Geometry Shader////
	//Set POINTS primitives as INPUT
    glProgramParameteriEXT(programObject,GL_GEOMETRY_INPUT_TYPE_EXT , GL_POINTS );
	//Set TRIANGLE STRIP as OUTPUT
	glProgramParameteriEXT(programObject,GL_GEOMETRY_OUTPUT_TYPE_EXT , GL_TRIANGLE_STRIP);
	//Set maximum number of vertices to be generated by Geometry Shader to 16
	//16 is the maximum number of vertices a marching cube configuration can own
	//This parameter is very important and have an important impact on Shader performances
	//Its value must be chosen closer as possible to real maximum number of vertices
	glProgramParameteriEXT(programObject,GL_GEOMETRY_VERTICES_OUT_EXT, 16);


	//Link whole program object (Geometry+Vertex+Fragment)
	glLinkProgramARB(programObject);
	//Test link success
	GLint ok = false;
    glGetObjectParameterivARB(programObject, GL_OBJECT_LINK_STATUS_ARB, &ok);
	if (!ok){
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetInfoLogARB(programObject, maxLength, &maxLength, infoLog);
		std::cout<<"Link error: "<<infoLog<<"\n";
		delete []infoLog;
	}

    //Program validation
    glValidateProgramARB(programObject);
    ok = false;
    glGetObjectParameterivARB(programObject, GL_OBJECT_VALIDATE_STATUS_ARB, &ok);
    if (!ok){
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetInfoLogARB(programObject, maxLength, &maxLength, infoLog);
		std::cout<<"Validation error: "<<infoLog<<"\n";
		delete []infoLog;
	}

	//Bind program object for parameters setting
	glUseProgramObjectARB(programObject);

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


	//Triangle Table texture//
	//This texture store the vertex index list for 
	//generating the triangles of each configurations.
	//(cf. MarchingCubes.cpp)
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

	//make the data 
	
	//Datafield//
	//Store the volume data to polygonise

	//Set current texture//
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_3D);
	// glBindTexture(GL_TEXTURE_3D, this->dataFieldTex[curData]);

	
	// create WorldBuilder Volume
	cgContext = cgCreateContext();
	m_wbSys = new WorldBuilderSystem(cgContext, dataSize.x, 
		dataSize.y, dataSize.z, 2, GL_RGBA32F_ARB, GL_RGBA, GL_FLOAT);
	glBindTexture(GL_TEXTURE_3D, m_wbSys->getStateBuffer()->getTexture());
	m_wbSys->getStateBuffer()->getTexture();
	//glTexImage3D( GL_TEXTURE_3D, 0, GL_ALPHA32F_ARB, dataSize.x, dataSize.y, dataSize.z, 0, GL_ALPHA, GL_FLOAT, dataField[0]);

	////Samplers assignment///
	glUniform1iARB(glGetUniformLocationARB(programObject, "dataFieldTex"), 0);
	glUniform1iARB(glGetUniformLocationARB(programObject, "edgeTableTex"), 1); 
  glUniform1iARB(glGetUniformLocationARB(programObject, "triTableTex"), 2); 

	////Uniforms parameters////
	//Initial isolevel
	glUniform1fARB(glGetUniformLocationARB(programObject, "isolevel"), isolevel); 
	//Step in data 3D texture for gradient computation (lighting)
	glUniform3fARB(glGetUniformLocationARB(programObject, "dataStep"), 1.0f/dataSize.x, 1.0f/dataSize.y, 1.0f/dataSize.z); 

	//Decal for each vertex in a marching cube
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[0]"), 0.0f, 0.0f, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[1]"), cubeStep.x, 0.0f, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[2]"), cubeStep.x, cubeStep.y, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[3]"), 0.0f, cubeStep.y, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[4]"), 0.0f, 0.0f, cubeStep.z); 
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[5]"), cubeStep.x, 0.0f, cubeStep.z); 
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[6]"), cubeStep.x, cubeStep.y, cubeStep.z); 
	glUniform3fARB(glGetUniformLocationARB(programObject, "vertDecals[7]"), 0.0f, cubeStep.y, cubeStep.z); 



	////////////////////////////////////////////////////////////////////////////////////////////
	///////Two others versions of the program////////
	//Geometry Shader only version (plus VS needed)//
	//Program object creation
	programObjectGS = glCreateProgramObjectARB();

	////Shaders loading////
	//Geometry Shader loading
	initShader(programObjectGS, "Shaders/TestG80_GS2.glsl", GL_GEOMETRY_SHADER_EXT);
	//Geometry Shader require a Vertex Shader to be used
	initShader(programObjectGS, "Shaders/TestG80_VS.glsl", GL_VERTEX_SHADER_ARB);
	//Fragment Shader for per-fragment lighting
	initShader(programObjectGS, "Shaders/TestG80_FS_Simple.glsl", GL_FRAGMENT_SHADER_ARB);

	////Setup Geometry Shader////
	//Set POINTS primitives as INPUT
    glProgramParameteriEXT(programObjectGS,GL_GEOMETRY_INPUT_TYPE_EXT , GL_POINTS );
	//Set TRIANGLE STRIP as OUTPUT
	glProgramParameteriEXT(programObjectGS,GL_GEOMETRY_OUTPUT_TYPE_EXT , GL_TRIANGLE_STRIP);
	//Set maximum number of vertices to be generated by Geometry Shader to 16
	//16 is the maximum number of vertices a marching cube configuration can own
	//This parameter is very important and have an important impact on Shader performances
	//Its value must be chosen closer as possible to real maximum number of vertices
	glProgramParameteriEXT(programObjectGS,GL_GEOMETRY_VERTICES_OUT_EXT, 16);

	//Link whole program object (Geometry+Vertex)
	glLinkProgramARB(programObjectGS);
	//Program validation
    glValidateProgramARB(programObjectGS);

	//Bind program object for parameters setting
	glUseProgramObjectARB(programObjectGS);

	////Samplers assignment///
	glUniform1iARB(glGetUniformLocationARB(programObjectGS, "dataFieldTex"), 0);
	glUniform1iARB(glGetUniformLocationARB(programObjectGS, "edgeTableTex"), 1); 
    glUniform1iARB(glGetUniformLocationARB(programObjectGS, "triTableTex"), 2); 

	////Uniforms parameters////
	//Initial isolevel
	glUniform1fARB(glGetUniformLocationARB(programObjectGS, "isolevel"), isolevel); 
	//Decal for each vertex in a marching cube
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[0]"), 0.0f, 0.0f, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[1]"), cubeStep.x, 0.0f, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[2]"), cubeStep.x, cubeStep.y, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[3]"), 0.0f, cubeStep.y, 0.0f); 
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[4]"), 0.0f, 0.0f, cubeStep.z); 
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[5]"), cubeStep.x, 0.0f, cubeStep.z); 
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[6]"), cubeStep.x, cubeStep.y, cubeStep.z); 
	glUniform3fARB(glGetUniformLocationARB(programObjectGS, "vertDecals[7]"), 0.0f, cubeStep.y, cubeStep.z); 
	/////

	//Fragment Shader only version for software marching cubes lighting//
	//Program object creation
	programObjectFS = glCreateProgramObjectARB();
	//Vertex Shader for software MC
	initShader(programObjectFS, "Shaders/TestG80_VS2.glsl", GL_VERTEX_SHADER_ARB);
	//Fragment Shader for per-fragment lighting
	initShader(programObjectFS, "Shaders/TestG80_FS.glsl", GL_FRAGMENT_SHADER_ARB);
	
	//Link whole program object (Geometry+Vertex)
	glLinkProgramARB(programObjectFS);
	//Program validation
    glValidateProgramARB(programObjectFS);

	//Bind program object for parameters setting
	glUseProgramObjectARB(programObjectFS);

	////Samplers assignment///
	glUniform1iARB(glGetUniformLocationARB(programObjectFS, "dataFieldTex"), 0);

	//Step in data 3D texture for gradient computation (lighting)
	glUniform3fARB(glGetUniformLocationARB(programObjectFS, "dataStep"), 1.0f/dataSize.x, 1.0f/dataSize.y, 1.0f/dataSize.z); 

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
	glGenBuffersARB(1, &gridDataBuffId);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, gridDataBuffId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, cubeSize.x*cubeSize.y*cubeSize.z*3*4, gridData, GL_STATIC_DRAW_ARB);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
	
	//Swizzled Walk 
	int n=0;
	swizzledWalk(n, gridData, vec3i(0,0,0), vec3i(cubeSize.x, cubeSize.y, cubeSize.z), cubeSize);


	//VBO configuration for marching grid Swizzled walk
	glGenBuffersARB(1, &gridDataSwizzledBuffId);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, gridDataSwizzledBuffId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, cubeSize.x*cubeSize.y*cubeSize.z*3*4, gridData, GL_STATIC_DRAW_ARB);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
}


void FionaMarchingCube::initShader(GLhandleARB programObject, const char *filen, GLuint type)
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

	const GLcharARB *txt=buff.c_str();

	//Shader object creation
	GLhandleARB object = glCreateShaderObjectARB(type);
	
	//Source code assignment
	glShaderSourceARB(object, 1, &txt, NULL);

	//Compile shader object
	glCompileShaderARB(object);

	//Check if shader compiled
	GLint ok = 0;
	glGetObjectParameterivARB(object, GL_OBJECT_COMPILE_STATUS_ARB, &ok);
	if (!ok){
		int maxLength=4096;
		char *infoLog = new char[maxLength];
		glGetInfoLogARB(object, maxLength, &maxLength, infoLog);
		std::cout<<"Compilation error: "<<infoLog<<"\n";
		delete []infoLog;
	}

	// attach shader to program object
	glAttachObjectARB(programObject, object);

	// delete object, no longer needed
	glDeleteObjectARB(object);

	//Global error checking
	std::cout<<"InitShader: "<<filen<<" Errors:"<<gluErrorString(glGetError())<<"\n";
}

void FionaMarchingCube::swizzledWalk(int &n, float *gridData, vec3i pos, vec3i size, const vec3 &cubeSize)
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

void FionaMarchingCube::render(void)
{
	if(!initialized)
	{
		init();
		initialized=true;
	}
	
	FionaScene::render();
    
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
  if (m_cubeWireFrame) {
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 1.0f);
    glutWireCube(m_cubeScale*2.0f);
    glEnable(GL_LIGHTING);
    glPopMatrix();
  }                                                        

  // draw simple indicator (if in bounds?)
   vec3 pos = this->getNormalizedWandPos();
   if (inBounds(pos)) {
	this->drawIndicator();
   }

    //draw second tracker indicator..
    drawSecondTracker();

	bool doWiiFitRotation = true;
	//begin wii-fit rotation
	if(doWiiFitRotation)
	{
		glPushMatrix();
		glRotatef(m_wiiFitRotation, 0.f, 1.f, 0.f);
	}

   	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_3D);
   
	glDepthMask(GL_TRUE);
	//glClearColor(0.0,0.0,0.0,0.0);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//States setting
	//start fionaut
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_ALPHA_TEST);
	//end fionaut

		//start fionaut
	glColor4f(cosf(isolevel*10.0-0.5), sinf(isolevel*10.0-0.5), cosf(1.0-isolevel),1.0);

	if(mode!=0){
		if(mode==1){
			//Shader program binding
			glUseProgramObjectARB(programObject);
			//Current isolevel uniform parameter setting
			glUniform1fARB(glGetUniformLocationARB(programObject, "isolevel"), isolevel); 
		}else{
			//Shader program binding
			glUseProgramObjectARB(programObjectGS);
			//Current isolevel uniform parameter setting
			glUniform1fARB(glGetUniformLocationARB(programObjectGS, "isolevel"), isolevel); 
		}	

		//glEnable(GL_LIGHTING);

		//Switch to wireframe or solid rendering mode
		if(wireframe)
			glPolygonMode(GL_FRONT_AND_BACK , GL_LINE );
		else
			glPolygonMode(GL_FRONT_AND_BACK , GL_FILL );

		if(!enableVBO){
			//Initial geometries are points. One point is generated per marching cube.
			glScalef(m_cubeScale, m_cubeScale, m_cubeScale);
			glBegin(GL_POINTS);
				float tmp = 1.0;
				for(float k=-tmp; k<tmp; k+=cubeStep.z)
				for(float j=-tmp; j<tmp; j+=cubeStep.y)
				for(float i=-tmp; i<tmp; i+=cubeStep.x){
							glVertex3f(i, j, k);	
				}
			glEnd();
		}else{
			///VBO
			if(enableSwizzledWalk)
				glBindBuffer(GL_ARRAY_BUFFER_ARB, gridDataSwizzledBuffId);
			else
				glBindBuffer(GL_ARRAY_BUFFER_ARB, gridDataBuffId);

			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0,  NULL);
			glDrawArrays(GL_POINTS, 0, cubeSize.x*cubeSize.y*cubeSize.z);
			glDisableClientState(GL_VERTEX_ARRAY);

			glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		}

			//glDisable(GL_LIGHTING);

			//Disable shader program
		glUseProgramObjectARB(NULL);
	}else{
	#if 1
		//SOFTWARE mode
		//Shader program binding
		glUseProgramObjectARB(programObjectFS);

		renderMarchCube(m_volumedata, vec3i(128,128,128), vec3i(cubeSize.x, cubeSize.y, cubeSize.z), isolevel);
		//Disable shader program
		glUseProgramObjectARB(NULL);
	#endif
		}

		//end fionaut

		//Automatic animation
		//start fionaut	- this would have to be clustered and done through sending a network message..
		if(animate){
			if(autoWay){
				if(isolevel<1.0)
					isolevel+=0.005;
				else
					autoWay=!autoWay;
			}else{
				if(isolevel>0.0)
					isolevel-=0.005;
				else
					autoWay=!autoWay;
			}
		}
		//endfionaut
	//}
	if(doWiiFitRotation)
	{
		glPopMatrix();
	}
  if( FionaIsFirstOfCycle() )
	this->addBlob();
}

int FionaMarchingCube::edgeTable[256]={
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

int FionaMarchingCube::triTable[256][16] =
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

void FionaMarchingCube::renderMarchCube(float *data, vec3i size, vec3i gridsize, float isolevel)
{
	vec3 gridStep=vec3(2.0/(float)gridsize.x,2.0/(float)gridsize.y,2.0/(float)gridsize.z);

	vec3i dataGridStep(size.x/gridsize.x, size.y/gridsize.y, size.z/gridsize.z);

	vec3 *triangles=new vec3[16];

	for(int k=0; k<gridsize.z-1; k++)
		for(int j=0; j<gridsize.y-1; j++)
			for(int i=0; i<gridsize.x-1; i++){
				GridCell cell;
				vec3 vcurf(i, j, k);
				vec3i vcuri(i, j, k);

				cell.pos[0]=vcurf*gridStep-vec3(1.0f, 1.0f, 1.0f);
				vec3i valPos0=vcuri*dataGridStep;
				cell.val[0]=data[valPos0.x + valPos0.y*size.x + valPos0.z*size.x*size.y];

				vec3i valPos;

				cell.pos[1]=cell.pos[0]+vec3(gridStep.x, 0, 0);
				if(i==gridsize.x-1)
					valPos=valPos0;
				else
					valPos=valPos0+vec3i(dataGridStep.x, 0, 0);
				cell.val[1]=data[valPos.x + valPos.y*size.x + valPos.z*size.x*size.y];

				cell.pos[2]=cell.pos[0]+vec3(gridStep.x, gridStep.y, 0);
				valPos=valPos0+vec3i(i==gridsize.x-1 ? 0 : dataGridStep.x, j==gridsize.y-1 ? 0 : dataGridStep.y, 0);
				cell.val[2]=data[valPos.x + valPos.y*size.x + valPos.z*size.x*size.y];

				cell.pos[3]=cell.pos[0]+vec3(0, gridStep.y, 0);
				valPos=valPos0+vec3i(0, j==gridsize.y-1 ? 0 : dataGridStep.y, 0);
				cell.val[3]=data[valPos.x + valPos.y*size.x + valPos.z*size.x*size.y];



				cell.pos[4]=cell.pos[0]+vec3(0, 0, gridStep.z);
				valPos=valPos0+vec3i(0, 0, k==gridsize.z-1 ? 0 : dataGridStep.z);
				cell.val[4]=data[valPos.x + valPos.y*size.x + valPos.z*size.x*size.y];


				cell.pos[5]=cell.pos[0]+vec3(gridStep.x, 0, gridStep.z);
				valPos=valPos0+vec3i(i==gridsize.x-1 ? 0 : dataGridStep.x, 0, k==gridsize.z-1 ? 0 : dataGridStep.z);
				cell.val[5]=data[valPos.x + valPos.y*size.x + valPos.z*size.x*size.y];

				cell.pos[6]=cell.pos[0]+vec3(gridStep.x, gridStep.y, gridStep.z);
				valPos=valPos0+vec3i(i==gridsize.x-1 ? 0 : dataGridStep.x, j==gridsize.y-1 ? 0 : dataGridStep.y, k==gridsize.z-1 ? 0 : dataGridStep.z);
				cell.val[6]=data[valPos.x + valPos.y*size.x + valPos.z*size.x*size.y];

				cell.pos[7]=cell.pos[0]+vec3(0, gridStep.y, gridStep.z);
				valPos=valPos0+vec3i(0, j==gridsize.y-1 ? 0 : dataGridStep.y, k==gridsize.z-1 ? 0 : dataGridStep.z);
				cell.val[7]=data[valPos.x + valPos.y*size.x + valPos.z*size.x*size.y];


				int numvert=polygonise(cell, isolevel, triangles);


				glBegin(GL_TRIANGLES);

				for(int n=0; n<numvert; n++){
					glVertex3f(triangles[n].x, triangles[n].y, triangles[n].z);
				}

				glEnd();

			}
}

/*
   Linearly interpolate the position where an isosurface cuts
   an edge between two vertices, each with their own scalar value
*/
vec3 FionaMarchingCube::vertexInterp(float isolevel, vec3 p1, vec3 p2, float valp1, float valp2) {
   float mu;
   vec3 p;

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

int FionaMarchingCube::polygonise(GridCell &grid, float isolevel, vec3 *triangles)
{
   int i,ntriang;
   int cubeindex;
   vec3 vertlist[12];
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

   /* Cube is entirely in/out of the surface */
   if (edgeTable[cubeindex] == 0)
      return(0);

   /* Find the vertices where the surface intersects the cube */
   if (edgeTable[cubeindex] & 1)
      vertlist[0] =
         vertexInterp(isolevel,grid.pos[0],grid.pos[1],grid.val[0],grid.val[1]);
   if (edgeTable[cubeindex] & 2)
      vertlist[1] =
         vertexInterp(isolevel,grid.pos[1],grid.pos[2],grid.val[1],grid.val[2]);
   if (edgeTable[cubeindex] & 4)
      vertlist[2] =
         vertexInterp(isolevel,grid.pos[2],grid.pos[3],grid.val[2],grid.val[3]);
   if (edgeTable[cubeindex] & 8)
      vertlist[3] =
         vertexInterp(isolevel,grid.pos[3],grid.pos[0],grid.val[3],grid.val[0]);
   if (edgeTable[cubeindex] & 16)
      vertlist[4] =
         vertexInterp(isolevel,grid.pos[4],grid.pos[5],grid.val[4],grid.val[5]);
   if (edgeTable[cubeindex] & 32)
      vertlist[5] =
         vertexInterp(isolevel,grid.pos[5],grid.pos[6],grid.val[5],grid.val[6]);
   if (edgeTable[cubeindex] & 64)
      vertlist[6] =
         vertexInterp(isolevel,grid.pos[6],grid.pos[7],grid.val[6],grid.val[7]);
   if (edgeTable[cubeindex] & 128)
      vertlist[7] =
         vertexInterp(isolevel,grid.pos[7],grid.pos[4],grid.val[7],grid.val[4]);
   if (edgeTable[cubeindex] & 256)
      vertlist[8] =
         vertexInterp(isolevel,grid.pos[0],grid.pos[4],grid.val[0],grid.val[4]);
   if (edgeTable[cubeindex] & 512)
      vertlist[9] =
         vertexInterp(isolevel,grid.pos[1],grid.pos[5],grid.val[1],grid.val[5]);
   if (edgeTable[cubeindex] & 1024)
      vertlist[10] =
         vertexInterp(isolevel,grid.pos[2],grid.pos[6],grid.val[2],grid.val[6]);
   if (edgeTable[cubeindex] & 2048)
      vertlist[11] =
         vertexInterp(isolevel,grid.pos[3],grid.pos[7],grid.val[3],grid.val[7]);

   /* Create the triangle */
   ntriang = 0;
   for (i=0;triTable[cubeindex][i]!=-1;i+=3) {
      triangles[ntriang] = vertlist[triTable[cubeindex][i  ]];
      triangles[ntriang+1] = vertlist[triTable[cubeindex][i+1]];
      triangles[ntriang+2] = vertlist[triTable[cubeindex][i+2]];
      ntriang+=3;
   }

   return(ntriang);
}

/*
void GLAppTestG80::keyboard(unsigned char key) {
  Vector3f col;

  switch (key) {
	case 'q':
		exit(0);
		break;
	case 'w':
		wireframe=!wireframe;
		this->dispText(2000, std::string("Wireframe: ")+this->wireframe, Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));

		break;
	case 'a':
		animate=!animate;
		this->dispText(2000, std::string("Animate: ")+this->animate, Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		break;
	case 'd':
		curData=(curData+1)%3;

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, this->dataFieldTex[curData]);

		this->dispText(2000, std::string("Data: ")+this->curData, Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		break;

	case '+':
		isolevel+=0.01;
		this->dispText(2000, std::string("Isolevel: ")+this->isolevel, Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		break;
	case '-':
		isolevel-=0.01;
		this->dispText(2000, std::string("Isolevel: ")+this->isolevel, Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		break;

	case 'm':
		mode=(mode+1)%3;
		if(mode==0)
			this->dispText(2000, std::string("Software Mode"), Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		else if(mode==1)
			this->dispText(2000, std::string("Geometry Shader Standard Mode"), Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		else if(mode==2)
			this->dispText(2000, std::string("Geometry Shader Performance Mode"), Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));

		break;
	case 'v':
		enableVBO=!enableVBO;
		this->dispText(2000, std::string("VBO: ")+this->enableVBO, Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		break;
	case 's':
		enableSwizzledWalk=!enableSwizzledWalk;
		this->dispText(2000, std::string("SwizzledWalk: ")+this->enableSwizzledWalk, Vector2i(200,20), Vector4f(1.0f, 1.0f, 0.0f, 1.0f));
		break;
	case ',':
		// Decrease disparity
		DISPARITY -= 0.001;
		break;
	case '.':
		// Increase disparity, duh, worthless comment.
		DISPARITY += 0.001;
		break;
  }
}
*/
