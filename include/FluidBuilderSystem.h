#ifndef FLUIDBUILDERSYSTEM
#define FLUIDBUILDERSYSTEM

#include "WorldBuilderSystem.h"
#include <vector>

class FluidBuilderSystem : public WorldBuilderSystem {
public:

	static const int gNumStates = 2;

	FluidBuilderSystem(CGcontext context, int width, int height, int depth, 
		GLint internalformat, GLenum format=GL_RGBA, GLenum type=GL_FLOAT);
	FluidBuilderSystem(CGcontext context, int width, int height, int depth, GLuint texid, float *data,
		GLint internalformat, GLenum format=GL_RGBA, GLenum type=GL_FLOAT);
	FluidBuilderSystem(CGcontext context, int width, int height, int depth, int banks,
		GLint internalformat, GLenum format=GL_RGBA, GLenum type=GL_FLOAT);
	~FluidBuilderSystem();

	void reset();
	//void add(float x = 0.0f, float y = 0.0f, float z = 0.0f, float radius=0.5f, float scale=1.0f);
	void add(float x = 0.0f, float y = 0.0f, float z = 0.0f, 
          float radius=0.5f, float scale=1.0f, int mode=0,
		  float r=0.0f, float g=0.4f, float b=0.4f);
	void force(float x = 0.0f, float y = 0.0f, float z = 0.0f, float radius=0.5f, float scale=100.0f);
  void simulate(float timestep=0.1);
	void fluid(float timestep);
  void switchBuffer();

  VolumeBuffer *getStateBuffer() { return m_State; };
	VolumeBuffer *getDivergenceBuffer() { return m_Divergence; };

private:

  int m_currdensity, m_currpressure, m_currvelocity, m_currobstacles;

	VolumeBuffer *m_Fluid;		    // 4float, velocities and densities
	VolumeBuffer *m_Density;		  // 1float, densities
	VolumeBuffer *m_Pressure;		  // 1float, densities
	VolumeBuffer *m_Velocity;		  // 3float, velocities
	VolumeBuffer *m_Phi_N;		    // 1float interm advection vals
	VolumeBuffer *m_Phi_N_1;		  // 1float interm advection vals
	VolumeBuffer *m_Divergence;	  // 1float, div
	VolumeBuffer *m_Obstacles;		// 4float, vel and densities

	Program	*m_set_prog,
      *m_addforce_prog,
      *m_boundaries_prog,
      *m_brownian_prog,
			*m_advect_prog,
			*m_advectmac_prog,
			*m_divergence_prog,
			*m_jacobi_prog,
			*m_project_prog ;

  void createPrograms();
  void bindCurrVB();
  void unbindCurrVB();
};
#endif
