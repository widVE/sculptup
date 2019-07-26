/**** Geometry Shader Marching Cubes
	* Copyright Cyril Crassin, January 2007.
	* This code is partially based on the example of 
	* Paul Bourke "Polygonising a scalar field" located at :
	* http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonise/
****/

#include "WorldBuilderG80.h"
#include <time.h>
#include <math.h>
#include <string>
#include <iostream>
#include <fstream>

extern int edgeTable[256];
extern int triTable[256][16];

//med data

WorldBuilderG80::WorldBuilderG80(const char *title, int w, int h, int flags):
						GLAppTestG80(title, w, h, flags) {
	
}
WorldBuilderG80::WorldBuilderG80(int *argcp, char **argv, const char *title, int w, int h, int flags):
						GLAppTestG80(argcp, argv, title, w, h, flags) {
	
	mouseButtons[0]=mouseButtons[1]=mouseButtons[2]=0;
	pause=false;

	this->wireframe=false;;

	viewOrient=Vector3f(1,0,1.5);

	cubeSize=Vector3f(64,64,64);
	cubeStep=Vector3f(2.0f, 2.0f, 2.0f)/(cubeSize);

	dataSize=Vector3i(128, 128, 128);

	isolevel=0.5f;
	animate=false;
	autoWay=true;
	curData=0;
	mode=1;
	enableSwizzledWalk=true;

}

void WorldBuilderG80::keyboard(unsigned char key) {
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
		wbSys->getStateBuffer()->setTexID(this->dataFieldTex[curData]);

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
	// WorldBuildeer
	case 'r':
		wbSys->reset();
		break;
	case 'x': 
		{
		float x = rand()/(float)RAND_MAX, 
			y = rand()/(float)RAND_MAX,
			z = rand()/(float)RAND_MAX,
			rad = rand()/(float)RAND_MAX;
		wbSys->add(x,y,z,rad);
		}
		break;
	case 'f':
		wbSys->fill();
		break;
  }
}


void WorldBuilderG80::mouse(int button, int state, int x, int y){
	oldMousePos.x=x;
	oldMousePos.y=y;

	switch(button){
	case GLUT_LEFT_BUTTON:
		this->mouseButtons[0] = !state;
		break;
	case GLUT_RIGHT_BUTTON:
		this->mouseButtons[2] = !state;
		break;
	case GLUT_MIDDLE_BUTTON:
		this->mouseButtons[1] = !state;
		break;
	}
}


void WorldBuilderG80::mouseMove(int x, int y){

	float rx =(float)(x-oldMousePos.x);
	float ry =(float)(y-oldMousePos.y);

	Vector2i wMove=Vector2i( (x-oldMousePos.x)/1, 0-(y-oldMousePos.y)/1);

	if(mouseButtons[0]){
		this->viewOrient.x+=ry*0.1;
		this->viewOrient.y+=rx*0.1;
	}else if(mouseButtons[1]){
	}else if(mouseButtons[2]){	
		isolevel+=rx*0.01;
	}
	
	oldMousePos.x = x;
	oldMousePos.y = y;
}

void WorldBuilderG80::mousePassiveMove(int x, int y){

}

void WorldBuilderG80::perspectiveGL( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar ) {
	const GLdouble pi = 3.1415926535897932384626433832795;
	GLdouble fW, fH;
	fH = tan( fovY / 360 * pi ) * zNear;
	fW = fH * aspect;
	glFrustum( -fW, fW, -fH, fH, zNear, zFar );
}


void WorldBuilderG80::reshape(int w, int h) { 
  glMatrixMode(GL_PROJECTION );
  glLoadIdentity();
  perspectiveGL(60.0, (float) w / (float) h, 0.10, 1000.0);

  glMatrixMode(GL_MODELVIEW);
  glViewport(0, 0, w, h);
}


void WorldBuilderG80::idle() { 
	
	this->redisplay();
}

void WorldBuilderG80::display() {

		//Viewport clearing
		
	glViewport(0, 0, this->windowSize.x,this->windowSize.y);
	glDepthMask(GL_TRUE);
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//States setting
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glDisable(GL_ALPHA_TEST);

		for (int lame=0; lame<2; lame++)
		{
		int wy = .5*this->windowSize.y;
		int w = this->windowSize.x;
		int h = wy;
		 glMatrixMode(GL_PROJECTION );
		glLoadIdentity();
		perspectiveGL(60.0, (float) this->windowSize.x / (float) this->windowSize.y, 0.10, 1000.0);
		//Activate modelview
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glPushMatrix();

		// Side by side in landscape orientation
		// glViewport(lame * this->windowSize.x / 2, 0, this->windowSize.x / 2, this->windowSize.y);

		// Top and bottom in portrait orientation
		glViewport(0, lame * this->windowSize.y / 2, this->windowSize.x, this->windowSize.y / 2);

		//View positioning
		
		glTranslatef(DISPARITY - lame * DISPARITY * 2,0,-3);

		glRotatef(this->viewOrient.x*10, 1,0,0);
		glRotatef(this->viewOrient.y*10, 0,1,0);

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
				glBegin(GL_POINTS);
					for(float k=-1; k<1.0f; k+=cubeStep.z)
					for(float j=-1; j<1.0f; j+=cubeStep.y)
					for(float i=-1; i<1.0f; i+=cubeStep.x){
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
			//Shader program binding
			glUseProgramObjectARB(programObjectFS);

			RenderMarchCube(dataField[curData], Vector3i(128,128,128), Vector3i(cubeSize.x, cubeSize.y, cubeSize.z), isolevel);
			//Disable shader program
			glUseProgramObjectARB(NULL);
	#endif
		}

		glPopMatrix();

		//Automatic animation
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

	}
	glViewport(0, 0, this->windowSize.x,this->windowSize.y);
 //Buffer automatically swapped
}

void WorldBuilderG80::init(){
	glShadeModel (GL_SMOOTH);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	//Form multi-face view
	glDisable(GL_CULL_FACE);

	glDepthMask(true);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClearDepth(1.0);


	this->showFPS(true);

	//Glew extension manager initialisation
	glewInit();

	loadDataFile("CT_head.hdr", "CT_head.dat");
	
	//Print Avalaible OpenGL extensions
	std::cout<<glGetString(GL_EXTENSIONS)<<"\n";

//Disable Vsync on windows plateform
#ifdef WIN32
	wglSwapIntervalEXT(0);
#endif

	srand(time(0));


	/////GLSL/////

	//Program object creation
	programObject = glCreateProgramObjectARB();


	////Shaders loading////
	//Geometry Shader loading
	initShader(programObject, "shaders/TestG80_GS2.glsl", GL_GEOMETRY_SHADER_EXT);
	//Geometry Shader require a Vertex Shader to be used
	initShader(programObject, "shaders/TestG80_VS.glsl", GL_VERTEX_SHADER_ARB);
	//Fragment Shader for per-fragment lighting
	initShader(programObject, "shaders/TestG80_FS.glsl", GL_FRAGMENT_SHADER_ARB);
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


	//Datafield//
	//Store the volume data to polygonise
	glGenTextures(3, (this->dataFieldTex));

	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, this->dataFieldTex[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//Generate a distance field to the center of the cube
	/*dataField[0]=new float[dataSize.x*dataSize.y*dataSize.z];
	for(int k=0; k<dataSize.z; k++)
	for(int j=0; j<dataSize.y; j++)
	for(int i=0; i<dataSize.x; i++){
		float d=Vector3f(i, j, k).distance(Vector3f(dataSize.x/2,dataSize.y/2,dataSize.z/2))/(float)(dataSize.length()*0.4);
		dataField[0][i+j*dataSize.x+k*dataSize.x*dataSize.y]=d;//+(rand()%100-50)/200.0f*d;
	}*/

	dataSize.x = m_datawidth;
	dataSize.y = m_dataheight;
	dataSize.z = m_datadepth;


	dataField[0]=new float[m_datawidth *m_dataheight *m_datadepth];
	for(int k=0; k<dataSize.z; k++)
	for(int j=0; j<dataSize.y; j++)
	for(int i=0; i<dataSize.x; i++){
		float d= getDataValue(i,j,k) / 255.;
		dataField[0][i+j*dataSize.x+k*dataSize.x*dataSize.y]=d;//+(rand()%100-50)/200.0f*d;
	}
 

	glTexImage3D( GL_TEXTURE_3D, 0, GL_ALPHA32F_ARB, dataSize.x, dataSize.y, dataSize.z, 0, GL_ALPHA, GL_FLOAT, dataField[0]);

	//Datafield Perturbated//
	dataField[1]=new float[dataSize.x*dataSize.y*dataSize.z];
	//perturb
	for(int k=0; k<dataSize.z; k++)
	for(int j=0; j<dataSize.y; j++)
	for(int i=0; i<dataSize.x; i++){
		float d=dataField[0][i+j*dataSize.x+k*dataSize.x*dataSize.y];
		dataField[1][i+j*dataSize.x+k*dataSize.x*dataSize.y]=d+(rand()%100-50)/100.0f*d;
	}

	//Smooth
	for(int l=0; l<4; l++)
	for(int k=1; k<dataSize.z-1; k++)
	for(int j=1; j<dataSize.y-1; j++)
	for(int i=1; i<dataSize.x-1; i++){
		dataField[1][i+j*dataSize.x+k*dataSize.x*dataSize.y]=(dataField[1][i+1+j*dataSize.x+k*dataSize.x*dataSize.y]+dataField[1][i-1+j*dataSize.x+k*dataSize.x*dataSize.y]+dataField[1][i+(j+1)*dataSize.x+k*dataSize.x*dataSize.y]+dataField[1][i+(j-1)*dataSize.x+k*dataSize.x*dataSize.y]+dataField[1][i+j*dataSize.x+(k+1)*dataSize.x*dataSize.y]+dataField[1][i+j*dataSize.x+(k-1)*dataSize.x*dataSize.y])/6.0f;
	}

	//Store the volume data to polygonise
	glBindTexture(GL_TEXTURE_3D, this->dataFieldTex[1]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//glTexImage3D( GL_TEXTURE_3D, 0, GL_ALPHA32F_ARB, dataSize.x, dataSize.y, dataSize.z, 0, GL_ALPHA, GL_FLOAT, dataField[1]);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, m_datawidth,
 m_dataheight,
 m_datadepth, 0, GL_RED, 
             GL_UNSIGNED_BYTE, m_volumedata);


	//Cayley-polynomial//
	dataField[2]=new float[dataSize.x*dataSize.y*dataSize.z];

	for(int k=0; k<dataSize.z; k++)
	for(int j=0; j<dataSize.y; j++)
	for(int i=0; i<dataSize.x; i++){
		float x=2.0f/dataSize.x*i-1.0f;
		float y=2.0f/dataSize.y*j-1.0f;
		float z=2.0f/dataSize.z*k-1.0f;
		dataField[2][i+j*dataSize.x+k*dataSize.x*dataSize.y]= 16.0f*x*y*z + 4.0f*x*x + 4.0f*y*y + 4.0f*z*z - 1.0f;
	}

	glBindTexture(GL_TEXTURE_3D, this->dataFieldTex[2]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glTexImage3D( GL_TEXTURE_3D, 0, GL_ALPHA32F_ARB, dataSize.x, dataSize.y, dataSize.z, 0, GL_ALPHA, GL_FLOAT, dataField[2]);

	//Set current texture//
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_3D);
	glBindTexture(GL_TEXTURE_3D, this->dataFieldTex[curData]);

	// create WorldBuilder Volume
	cgContext = cgCreateContext();
	wbSys = new WorldBuilderSystem(cgContext, m_datawidth, 
		m_dataheight, m_datadepth, this->dataFieldTex[curData]);

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
	initShader(programObjectGS, "shaders/TestG80_GS2.glsl", GL_GEOMETRY_SHADER_EXT);
	//Geometry Shader require a Vertex Shader to be used
	initShader(programObjectGS, "shaders/TestG80_VS.glsl", GL_VERTEX_SHADER_ARB);
	//Fragment Shader for per-fragment lighting
	initShader(programObjectGS, "shaders/TestG80_FS_Simple.glsl", GL_FRAGMENT_SHADER_ARB);

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
	initShader(programObjectFS, "shaders/TestG80_VS2.glsl", GL_VERTEX_SHADER_ARB);
	//Fragment Shader for per-fragment lighting
	initShader(programObjectFS, "shaders/TestG80_FS.glsl", GL_FRAGMENT_SHADER_ARB);
	
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
	swizzledWalk(n, gridData, Vector3i(0,0,0), Vector3i(cubeSize.x, cubeSize.y, cubeSize.z), cubeSize);


	//VBO configuration for marching grid Swizzled walk
	glGenBuffersARB(1, &gridDataSwizzledBuffId);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, gridDataSwizzledBuffId);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, cubeSize.x*cubeSize.y*cubeSize.z*3*4, gridData, GL_STATIC_DRAW_ARB);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
}



void WorldBuilderG80::initShader(GLhandleARB programObject, const char *filen, GLuint type){

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