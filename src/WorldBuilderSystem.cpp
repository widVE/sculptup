#include <math.h>

#include "WorldBuilderSystem.h"
#include "Shader.h"
using std::vector;

WorldBuilderSystem::WorldBuilderSystem(CGcontext context, int width, int height, int depth, int banks,
        GLint internalformat, GLenum format, GLenum type)
:  m_width(width), m_height(height), m_depth(depth),
    m_current(0),
    m_states(banks)
{
    m_State = new VolumeBuffer(internalformat, width, height, depth, banks, format, type);

    createPrograms();

	//reset();
}

WorldBuilderSystem::~WorldBuilderSystem() {
    delete m_State;

	if (m_logfile)
	{
		fclose(m_logfile);
	}
}

void WorldBuilderSystem::reset() {
	m_State->clear(m_clear_prog);
}

void WorldBuilderSystem::readResults()
{
	//m_State->runProgramReadResults(m_add_prog, m_State->getTexture(m_current));
	m_State->readResults();
}

void WorldBuilderSystem::add(
        float x, float y, float z, 
        float radius, float scale, int mode,
		float r, float g, float b, bool readResults)
{
	
	if (radius == 0.f)
	{
		radius = 0.1f;
	}
	
	if(!readResults)
	{
		m_State->sculpt(m_add_prog, m_current, x, y, z, r, g, b, radius, scale);
	}
	else
	{
		//m_State->runProgramReadResults(m_add_prog, m_current);
	}

	m_current = (m_current) ? 0 : 1;
}

void WorldBuilderSystem::addCube(
        float p1_x, float p1_y, float p1_z, 
        float p2_x, float p2_y, float p2_z, float r, float g, float b) 
{
	m_State->addCube(m_addcube_prog, m_current, p1_x, p1_y, p1_z, p2_x, p2_y, p2_z, r, g, b);
	m_current = (m_current) ? 0 : 1;
}

void WorldBuilderSystem::fill(float thresh) 
{
	m_State->fill(m_fill_prog, m_current, thresh);
	m_current = (m_current) ? 0 : 1;
}

void WorldBuilderSystem::paint(
        float x, float y, float z, 
        float r, float g, float b,
        float radius, float scale, int mode) 
{
    m_State->paint(m_paint_prog, m_current, x, y, z, r, g, b, radius, scale);
	m_current = (m_current) ? 0 : 1;
}

void WorldBuilderSystem::erase(
        float p1_x, float p1_y, float p1_z, 
        float p2_x, float p2_y, float p2_z, 
        float radius, float scale, int mode)
{
	m_State->erase(m_erase_prog, m_current, p1_x, p1_y, p1_z, p2_x, p2_y, p2_z, radius, 0.05f);
	m_current = (m_current) ? 0 : 1;
}

void WorldBuilderSystem::createPrograms() {
    // load programs
    m_clear_prog = new Shader("Shaders/dummy.vs", "Shaders/post.gs", "Shaders/clear.frag");
	m_add_prog = new Shader("Shaders/dummy.vs", "Shaders/post.gs", "Shaders/splat.frag");
	m_addcube_prog = new Shader("Shaders/dummy.vs", "Shaders/post.gs", "Shaders/cube.frag");
	m_fill_prog = new Shader("Shaders/dummy.vs", "Shaders/post.gs", "Shaders/fill.frag");
	m_paint_prog = new Shader("Shaders/dummy.vs", "Shaders/post.gs", "Shaders/paint.frag");
	m_erase_prog = new Shader("Shaders/dummy.vs", "Shaders/post.gs", "Shaders/erase.frag");
}

//for logging
void WorldBuilderSystem::createLog(std::string dir)
{
	std::string name = dir;
	name+="/log.txt";

	m_logfile = fopen(name.c_str(), "w");

}