#include <math.h>

#include "FluidBuilderSystem.h"

using std::vector;

FluidBuilderSystem::FluidBuilderSystem(CGcontext context, int width, int height, int depth,
        GLint internalformat, GLenum format, GLenum type)
: WorldBuilderSystem(context, width, height, depth, internalformat, format, type)
{
    m_Density = new VolumeBuffer(internalformat, width, height, depth, 2, format, type);
    createPrograms();

    reset();
}

FluidBuilderSystem::FluidBuilderSystem(CGcontext context, int width, int height, int depth, int banks,
        GLint internalformat, GLenum format, GLenum type)
: WorldBuilderSystem(context, width, height, depth, banks, internalformat, format, type),
    m_currdensity(0), m_currpressure(0), m_currvelocity(0), m_currobstacles(0)
{

    // m_Density = new VolumeBuffer(internalformat, width, height, depth, banks, format, type);


    // for (int i = 0; i < banks; i++) {
        // m_Density->runProgram(m_clear_prog, i);
    // }

    //m_Fluid = new VolumeBuffer(internalformat, width, height, depth, 2, format, type);
    m_Velocity = new VolumeBuffer(internalformat, width, height, depth, 2, format, type);
    // m_Velocity= new VolumeBuffer(GL_RGB32F, width, height, depth, 2, GL_RGB, GL_FLOAT);
    m_Density = m_State;
    // m_Density = new VolumeBuffer(internalformat, width, height, depth, 2, format, type);
    // m_Density = new VolumeBuffer(GL_ALPHA, width, height, depth, 2, GL_ALPHA, GL_FLOAT);
    m_Pressure = new VolumeBuffer(GL_ALPHA, width, height, depth, 2, GL_ALPHA, GL_FLOAT);
    m_Phi_N = new VolumeBuffer(GL_ALPHA, width, height, depth, 1, GL_ALPHA, GL_FLOAT);
    m_Phi_N_1 = new VolumeBuffer(GL_ALPHA, width, height, depth, 1, GL_ALPHA, GL_FLOAT);
    m_Divergence = new VolumeBuffer(GL_ALPHA, width, height, depth, 1, GL_ALPHA, GL_FLOAT);
    m_Obstacles = new VolumeBuffer(internalformat, width, height, depth, 2, format, type);

    createPrograms();
}

FluidBuilderSystem::~FluidBuilderSystem() {
    // delete m_Density;
    //delete m_Fluid;
}

void FluidBuilderSystem::reset() {
    // for (int i = 0; i < m_Density->getNumBanks(); ++i) {
        // m_Density->runProgram(m_clear_prog, i);
    // }
    // for (int i = 0; i < m_Fluid->getNumBanks(); ++i) {
        // m_Fluid->runProgram(m_set_prog, i);
    // }
   // m_set_prog->SetParameter("val", 0.0, -16.666, 0.0, 0.0);
   // m_set_prog->SetParameter("val", 0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < m_Velocity->getNumBanks(); ++i) {
        // m_Velocity->runProgram(m_set_prog, i);
        m_Velocity->runProgram(m_brownian_prog, i);
    }
    for (int i = 0; i < m_Obstacles->getNumBanks(); ++i) {
        m_Obstacles->runProgram(m_boundaries_prog, i);
    }
    /*m_set_prog->SetParameter("val", 0.0, 0.0, 0.0, 0.0);
    m_Phi_N->runProgram(m_set_prog);
    m_Phi_N_1->runProgram(m_set_prog);
    m_Divergence->runProgram(m_set_prog);
    for (int i = 0; i < m_Pressure->getNumBanks(); ++i) {
       m_Pressure->runProgram(m_set_prog, i);
    }*/
   // m_set_prog->SetParameter("val", 0.0, 0.0, 0.0, 0.0);
    for (int i = 0; i < m_Density->getNumBanks(); ++i) {
        m_Density->runProgram(m_clear_prog, i);
    }
}

//void FluidBuilderSystem::add(float x, float y, float z, float radius, float scale) {
void FluidBuilderSystem::add(
        float x, float y, float z, 
        float radius, float scale, int mode,
		float r, float g, float b) {
    m_add_prog->SetParameter("center", x, y, z);
    m_add_prog->SetParameter("radius", radius);
    m_add_prog->SetParameter("scale", scale);
    m_add_prog->SetTexture("tex", m_Density->getTexture(m_currdensity)); 
    m_currdensity = (m_currdensity) ? 0 : 1;
    m_Density->runProgram(m_add_prog, m_currdensity);
    // m_add_prog->SetTexture("tex", m_Pressure->getTexture(m_currpressure)); 
    // m_currpressure = (m_currpressure) ? 0 : 1;
    // m_Pressure->runProgram(m_add_prog, m_currpressure);
    this->bindCurrVB();    
}

void FluidBuilderSystem::force(float x, float y, float z, float radius, float scale) {
    m_addforce_prog->SetParameter("center", x, y, z);
    m_addforce_prog->SetParameter("radius", radius);
    m_addforce_prog->SetParameter("scale", scale);
    m_addforce_prog->SetTexture("tex", m_Velocity->getTexture(m_currvelocity)); 
    m_currvelocity = (m_currvelocity) ? 0 : 1;
    m_Velocity->runProgram(m_addforce_prog, m_currvelocity);
}

void FluidBuilderSystem::simulate(float timestep) {
    this->fluid(timestep);
}

void FluidBuilderSystem::fluid(float timestep) {
    timestep = 0.009;

    // ADVECTION
    // phi_n = current state (pass velocity/density vol)
    // 
    // Intermediate values
    // phi_n_1_hat = advect(vel);
    // phi_n_hat = advect_reverse(phi_n_1_hat);
    //
    // Output val
    // phi_n_1 = phi_n_1_hat + 0.5*(phi_n - phi_n_hat)
    m_advect_prog->SetTexture("velocity", m_Velocity->getTexture(m_currvelocity));
    m_advect_prog->SetTexture("phi", m_Density->getTexture(m_currdensity));
    m_advect_prog->SetTexture("obstacles", m_Obstacles->getTexture(m_currobstacles));
    m_advect_prog->SetParameter("timestep", timestep);
    // m_Phi_N_1->runProgram(m_advect_prog);
    // m_advect_prog->SetTexture("velocity", m_Velocity->getTexture(m_currvelocity));
    // m_advect_prog->SetTexture("phi", m_Phi_N_1->getTexture());
    // m_advect_prog->SetTexture("obstacles", m_Obstacles->getTexture(m_currobstacles));
    // m_advect_prog->SetParameter("timestep", -timestep);
    // m_Phi_N->runProgram(m_advect_prog);
    // m_advectmac_prog->SetTexture("velocity", m_Velocity->getTexture(m_currvelocity));
    // m_advectmac_prog->SetTexture("phi", m_Density->getTexture(m_currdensity));
    // m_advectmac_prog->SetTexture("phi_n_hat", m_Phi_N->getTexture());
    // m_advectmac_prog->SetTexture("phi_n_1_hat", m_Phi_N_1->getTexture());
	this->bindCurrVB();
    m_currdensity = (m_currdensity) ? 0 : 1;
    // m_Density->runProgram(m_advectmac_prog, m_currdensity);
    m_Density->runProgram(m_advect_prog, m_currdensity);

    m_divergence_prog->SetTexture("velocity", m_Velocity->getTexture(m_currvelocity));
    m_divergence_prog->SetTexture("obstacles", m_Obstacles->getTexture(m_currobstacles));
    m_Divergence->runProgram(m_divergence_prog);

    m_jacobi_prog->SetTexture("pressure", m_Pressure->getTexture(m_currpressure));
    m_jacobi_prog->SetTexture("div", m_Divergence->getTexture());
    m_jacobi_prog->SetTexture("obstacles", m_Obstacles->getTexture(m_currobstacles));
    m_currpressure = (m_currpressure) ? 0 : 1;
    m_Pressure->runProgram(m_jacobi_prog, m_currpressure);

    m_project_prog->SetTexture("vel", m_Velocity->getTexture(m_currvelocity));
    m_project_prog->SetTexture("pressure", m_Pressure->getTexture(m_currpressure));
    m_project_prog->SetTexture("obstacles", m_Obstacles->getTexture(m_currobstacles));
    m_currvelocity = (m_currvelocity) ? 0 : 1;
    m_Velocity->runProgram(m_project_prog, m_currvelocity);

    this->bindCurrVB();    
}

void FluidBuilderSystem::createPrograms() {

    WorldBuilderSystem::createPrograms();

    m_set_prog = new Program(m_cg_context, "shaders/simulate.cg", "set", CG_GL_FRAGMENT);
    m_addforce_prog = new Program(m_cg_context, "shaders/fluid.cg", "addrgb", CG_GL_FRAGMENT);
    m_brownian_prog = new Program(m_cg_context, "shaders/fluid.cg", "brownian", CG_GL_FRAGMENT);
    m_boundaries_prog = new Program(m_cg_context, "shaders/fluid.cg", "boundaries", CG_GL_FRAGMENT);
    m_advect_prog = new Program(m_cg_context, "shaders/fluid.cg", "advect", CG_GL_FRAGMENT);
    m_advectmac_prog = new Program(m_cg_context, "shaders/fluid.cg", "advectmac", CG_GL_FRAGMENT);
    m_jacobi_prog = new Program(m_cg_context, "shaders/fluid.cg", "jacobi", CG_GL_FRAGMENT);
    m_divergence_prog = new Program(m_cg_context, "shaders/fluid.cg", "diverg", CG_GL_FRAGMENT);
    m_project_prog = new Program(m_cg_context, "shaders/fluid.cg", "project", CG_GL_FRAGMENT);

    float x = 1.0/float(m_width);
    float y = 1.0/float(m_height);
    float z = 1.0/float(m_depth);
    m_advect_prog->SetParameter("voxelSize", x, y, z);
    m_advectmac_prog->SetParameter("voxelSize", x, y, z);
    m_jacobi_prog->SetParameter("voxelSize", x, y, z);
    m_divergence_prog->SetParameter("voxelSize", x, y, z);
    m_project_prog->SetParameter("voxelSize", x, y, z);
}

void FluidBuilderSystem::switchBuffer() {
    m_currdensity = (m_currdensity) ? 0 : 1;
    // m_Density->setCurrBank(m_currdensity);
    glActiveTexture(0);
    glBindTexture(GL_TEXTURE_3D, m_Density->getTexture(m_currdensity));
}

void FluidBuilderSystem::bindCurrVB() {
	glActiveTexture(0);
	glBindTexture(GL_TEXTURE_3D, m_State->getTexture(m_currdensity));
}
