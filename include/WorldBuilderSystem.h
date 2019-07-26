#ifndef WORLDBUILDERSYSTEM
#define WORLDBUILDERSYSTEM

#include "VolumeBuffer.h"

class Shader;

#include <vector>

class WorldBuilderSystem {
public:

	static const int gNumStates = 2;

	WorldBuilderSystem(CGcontext context, int width, int height, int depth, int banks,
		GLint internalformat, GLenum format=GL_RGBA, GLenum type=GL_FLOAT);
	~WorldBuilderSystem();

	void reset();

	virtual void add(float x = 0.0f, float y = 0.0f, float z = 0.0f, 
          float radius=0.5f, float scale=1.0f, int mode=0,
		  float r=0.0f, float g=0.4f, float b=0.4f, bool readResults=false);

  virtual void addCube(float p1_x, float p1_y, float p1_z, 
          float p2_x, float p2_y, float p2_z, float r, float g, float b);

  virtual void fill(float thresh = 0.1);

  virtual void paint(float x, float y, float z, float r, float g, float b,
          float radius=0.5f, float scale = 1.0f, int mode=0);

  virtual void erase(float p1_x, float p1_y, float p1_z, 
          float p2_x, float p2_y, float p2_z, 
          float radius=0.5f, float scale = 1.0f, int mode=0);

  VolumeBuffer *getStateBuffer() { return m_State; };

  void readResults(void);
  //for logging
  void createLog(std::string dir);
  
  int m_current;

protected:
  int m_width, m_height, m_depth;
  int m_states;

  VolumeBuffer *m_State;
  
  Shader *m_clear_prog,
	  *m_add_prog,
	  *m_addcube_prog,
	  *m_erase_prog,
	  *m_fill_prog,
	  *m_paint_prog;

  void createPrograms();

  //for logging
  FILE *m_logfile;

};

#endif