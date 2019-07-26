#include <math.h>
#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "VolumeBuffer.h"
#include "Shader.h"

VolumeBuffer::VolumeBuffer(GLint internalformat, int width, int height, int depth, int banks, 
	GLenum format, GLenum type)
    : m_width(width),
      m_height(height),
      m_depth(depth),
      m_banks(banks),
      m_blendMode(BLEND_NONE),
	  m_internalformat(internalformat),
	  m_format(format),
	  m_type(type),
	  m_geomData(0),
	  m_redData(0),
	  m_greenData(0),
	  m_blueData(0),
	  tmpData(0)
{
    // create fbo
    m_fbo = new FramebufferObject();

    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_max_banks);
    if (m_banks > m_max_banks) m_banks = m_max_banks;

    // create textures
    m_tex = new GLuint [m_banks];
    for(int i=0; i<m_banks; i++) {
        m_tex[i] = create3dTexture(m_internalformat, m_width, m_height, m_depth, m_format, m_type);
    }

	int memSize = m_width * m_height * m_depth;
	
	m_geomData = new float[memSize];
	memset(m_geomData, 0, sizeof(float) * memSize);
	m_redData = new float[memSize];
	memset(m_redData, 0, sizeof(float) * memSize);
	m_greenData = new float[memSize];
	memset(m_greenData, 0, sizeof(float) * memSize);
	m_blueData = new float[memSize];
	memset(m_blueData, 0, sizeof(float) * memSize);

    // attach slice 0 of first texture to fbo for starters
    /*m_fbo->Bind();
    m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[0], GL_COLOR_ATTACHMENT0, 0, 0);
    m_fbo->IsValid();
    m_fbo->Disable();*/
}

VolumeBuffer::VolumeBuffer(GLint internalformat, GLuint texid, int width, int height, int depth,
		GLenum format, GLenum type)
    : m_width(width),
      m_height(height),
      m_depth(depth),
      m_banks(1),
      m_blendMode(BLEND_NONE),
	  m_internalformat(internalformat),
	  m_format(format),
	  m_type(type),
	  m_geomData(0),
	  m_redData(0),
	  m_greenData(0),
	  m_blueData(0),
	  tmpData(0)
{
	m_fbo = new FramebufferObject();
	
	m_tex = new GLuint [1];
	m_tex[0] = texid;

	int memSize = m_width * m_height * m_depth;
	
	m_geomData = new float[memSize];
	memset(m_geomData, 0, sizeof(float) * memSize);
	m_redData = new float[memSize];
	memset(m_redData, 0, sizeof(float) * memSize);
	m_greenData = new float[memSize];
	memset(m_greenData, 0, sizeof(float) * memSize);
	m_blueData = new float[memSize];
	memset(m_blueData, 0, sizeof(float) * memSize);

	/*m_fbo->Bind();
    m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[0], GL_COLOR_ATTACHMENT0, 0, 0);
    m_fbo->IsValid();
    m_fbo->Disable();*/
}

VolumeBuffer::~VolumeBuffer()
{
    delete m_fbo;
    for(int i=0; i<m_banks; i++) {
        glDeleteTextures(1, &m_tex[i]);
    }
    delete [] m_tex;
	delete [] m_geomData;
	delete [] m_redData;
	delete [] m_greenData;
	delete [] m_blueData;
	if (tmpData)
		delete[] tmpData;
}

GLuint
VolumeBuffer::create3dTexture(GLint internalformat, int w, int h, int d, GLenum format, GLenum type)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint mode = GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, mode);
    glTexImage3D(GL_TEXTURE_3D, 0, internalformat, w, h, d, 0, format, type, 0);
    return tex;
}

void
VolumeBuffer::setWrapMode(GLint mode, int bank)
{
    glBindTexture(GL_TEXTURE_3D, m_tex[bank]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, mode);
}

void
VolumeBuffer::setFiltering(GLint mode, int bank)
{
    glBindTexture(GL_TEXTURE_3D, m_tex[bank]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, mode);
}

void 
VolumeBuffer::setData(unsigned char *data, int bank) 
{
    glBindTexture(GL_TEXTURE_3D, m_tex[bank]);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, m_width, m_height, m_depth, m_format, m_type, data);
}

void
VolumeBuffer::setData(float *data, int bank)
{
    glBindTexture(GL_TEXTURE_3D, m_tex[bank]);
	// copy data
	//int size = sizeof(data)/sizeof(float);
	//m_data = new float[size];
	//memcpy(m_data,data,size);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, m_width, m_height, m_depth, m_format, m_type, 0);
}

void 
VolumeBuffer::setTexID(GLuint texid, int bank) {
	m_tex[bank] = texid;
}

// draw a slice of the volume
void
VolumeBuffer::drawSlice(float z)
{
    glBegin(GL_QUADS);
    glTexCoord3f(0.0f, 0.0f, z); glVertex2f(-1.0f, -1.0f);
    glTexCoord3f(1.0f, 0.0f, z); glVertex2f(1.0f, -1.0f);
    glTexCoord3f(1.0f, 1.0f, z); glVertex2f(1.0f, 1.0f);
    glTexCoord3f(0.0f, 1.0f, z); glVertex2f(-1.0f, 1.0f);
    glEnd();
}

void VolumeBuffer::clear(Shader *fprog)
{
	for (int i = 0; i < getNumBanks(); ++i)
	{
		m_fbo->Bind();

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();
		glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
		printf("%d, %d\n", m_width, m_height);
		glViewport(0, 0, m_width, m_height);
		glDisable(GL_DEPTH_TEST);

		fprog->bind();

		for (int z = 0; z < m_depth; z++) {
			// attach texture slice to FBO
			fprog->setUniform("zSlice", ((float)z + 0.5f) / (float)m_depth);
			m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[i], GL_COLOR_ATTACHMENT0, 0, z);
			//std::cout << "After attach slices: " << gluErrorString(glGetError()) << "\n";
			// render
			//drawSlice((z + 0.5f) / (float)m_depth);
			glDrawArrays(GL_POINTS, 0, 1);
			//std::cout << "After draw slices: " << gluErrorString(glGetError()) << "\n";
		}

		fprog->disable();

		glDisable(GL_BLEND);

		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		m_fbo->Disable();
	}
}

void VolumeBuffer::fill(Shader *fprog, int bank, float thresh)
{
	m_fbo->Bind();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	//printf("%d, %d\n", m_width, m_height);
	glViewport(0, 0, m_width, m_height);
	glDisable(GL_DEPTH_TEST);

	/*switch(m_blendMode) {
	case BLEND_ADDITIVE:
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	break;
	}*/

	fprog->bind();

	fprog->setUniform("thresh", thresh);

	bank = (bank) ? 0 : 1;

	for (int z = 0; z<m_depth; z++) {
		// attach texture slice to FBO
		fprog->setUniform("zSlice", ((float)z + 0.5f) / (float)m_depth);
		m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[bank], GL_COLOR_ATTACHMENT0, 0, z);
		//std::cout << "After attach slices: " << gluErrorString(glGetError()) << "\n";
		// render
		//drawSlice((z + 0.5f) / (float)m_depth);
		glDrawArrays(GL_POINTS, 0, 1);
		//std::cout << "After draw slices: " << gluErrorString(glGetError()) << "\n";
	}

	fprog->disable();

	//glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	m_fbo->Disable();
}

void
VolumeBuffer::sculpt(Shader *fprog, int bank, float x, float y, float z, float r, float g, float b, float rad, float scale)
{
	m_fbo->Bind();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	//printf("%d, %d\n", m_width, m_height);
	glViewport(0, 0, m_width, m_height);
	glDisable(GL_DEPTH_TEST);

	/*switch(m_blendMode) {
	case BLEND_ADDITIVE:
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	break;
	}*/

	fprog->bind();
	
	fprog->setUniform("center", x, y, z);
	fprog->setUniform("color", r, g, b);
	fprog->setUniform("radius", rad);
	fprog->setUniform("scale", scale);

	fprog->setTexture3D("tex", getTexture(bank));

	bank = (bank) ? 0 : 1;

	for (int z = 0; z<m_depth; z++) {
		// attach texture slice to FBO
		fprog->setUniform("zSlice", ((float)z + 0.5f) / (float)m_depth);
		m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[bank], GL_COLOR_ATTACHMENT0, 0, z);
		//std::cout << "After attach slices: " << gluErrorString(glGetError()) << "\n";
		// render
		//drawSlice((z + 0.5f) / (float)m_depth);
		glDrawArrays(GL_POINTS, 0, 1);
		//std::cout << "After draw slices: " << gluErrorString(glGetError()) << "\n";
	}

	fprog->disable();

	//glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	m_fbo->Disable();
}

void VolumeBuffer::paint(Shader *fprog, int bank,
	float x, float y, float z,
	float r, float g, float b,
	float radius, float scale)
{
	m_fbo->Bind();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	//printf("%d, %d\n", m_width, m_height);
	glViewport(0, 0, m_width, m_height);
	glDisable(GL_DEPTH_TEST);

	/*switch(m_blendMode) {
	case BLEND_ADDITIVE:
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	break;
	}*/

	fprog->bind();

	fprog->setUniform("center", x, y, z);
	fprog->setUniform("color", r, g, b);
	fprog->setTexture3D("tex", getTexture(bank));
	fprog->setUniform("radius", radius);
	fprog->setUniform("scale", scale);

	bank = (bank) ? 0 : 1;

	for (int z = 0; z<m_depth; z++) {
		// attach texture slice to FBO
		fprog->setUniform("zSlice", ((float)z + 0.5f) / (float)m_depth);
		m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[bank], GL_COLOR_ATTACHMENT0, 0, z);
		glDrawArrays(GL_POINTS, 0, 1);
	}

	fprog->disable();

	//glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	m_fbo->Disable();
}

void VolumeBuffer::addCube(Shader *fprog, int bank,
	float p1_x, float p1_y, float p1_z,
	float p2_x, float p2_y, float p2_z, float r, float g, float b)
{
	m_fbo->Bind();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	//printf("%d, %d\n", m_width, m_height);
	glViewport(0, 0, m_width, m_height);
	glDisable(GL_DEPTH_TEST);

	/*switch(m_blendMode) {
	case BLEND_ADDITIVE:
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	break;
	}*/

	fprog->bind();

	fprog->setUniform("p1", p1_x, p1_y, p1_z);
	fprog->setUniform("p2", p2_x, p2_y, p2_z);
	fprog->setUniform("color", r, g, b);

	fprog->setTexture3D("tex", getTexture(bank));

	bank = (bank) ? 0 : 1;

	for (int z = 0; z<m_depth; z++) {
		// attach texture slice to FBO
		fprog->setUniform("zSlice", ((float)z + 0.5f) / (float)m_depth);
		m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[bank], GL_COLOR_ATTACHMENT0, 0, z);
		//std::cout << "After attach slices: " << gluErrorString(glGetError()) << "\n";
		// render
		//drawSlice((z + 0.5f) / (float)m_depth);
		glDrawArrays(GL_POINTS, 0, 1);
		//std::cout << "After draw slices: " << gluErrorString(glGetError()) << "\n";
	}

	fprog->disable();

	//glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	m_fbo->Disable();
}

void VolumeBuffer::erase(Shader *fprog, int bank,
	float p1_x, float p1_y, float p1_z,
	float p2_x, float p2_y, float p2_z,
	float radius, float thresh)
{
	m_fbo->Bind();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
	//printf("%d, %d\n", m_width, m_height);
	glViewport(0, 0, m_width, m_height);
	glDisable(GL_DEPTH_TEST);

	/*switch(m_blendMode) {
	case BLEND_ADDITIVE:
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	break;
	}*/

	fprog->bind();

	fprog->setUniform("p1", p1_x, p1_y, p1_z);
	fprog->setUniform("p2", p2_x, p2_y, p2_z);
	fprog->setUniform("radius", 0.05f);
	fprog->setUniform("thresh", thresh);

	fprog->setTexture3D("tex", getTexture(bank));

	bank = (bank) ? 0 : 1;

	for (int z = 0; z<m_depth; z++) {
		// attach texture slice to FBO
		fprog->setUniform("zSlice", ((float)z + 0.5f) / (float)m_depth);
		m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[bank], GL_COLOR_ATTACHMENT0, 0, z);
		//std::cout << "After attach slices: " << gluErrorString(glGetError()) << "\n";
		// render
		//drawSlice((z + 0.5f) / (float)m_depth);
		glDrawArrays(GL_POINTS, 0, 1);
		//std::cout << "After draw slices: " << gluErrorString(glGetError()) << "\n";
	}

	fprog->disable();

	//glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	m_fbo->Disable();
}

void VolumeBuffer::readResults()
{
	int memSize = m_width * m_height * m_depth * 4;
	if (tmpData == 0)
	{
		tmpData = new float[memSize];
		memset(tmpData, 0, sizeof(float) * memSize);
	}

	GLuint tid = this->getTexture();
	glBindTexture(GL_TEXTURE_3D, tid);

	glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, tmpData);
	int j = 0;
	for(int i = 0; i < memSize; i+=4)
	{
		m_geomData[j] = tmpData[i+3];
		m_redData[j] = tmpData[i];
		m_greenData[j] = tmpData[i+1];
		m_blueData[j] = tmpData[i+2];
		j++;
	}
}

/*
void VolumeBuffer::runProgramReadResults(Program *fprog, int bank)
{
	int memSize = m_width * m_height * m_depth * 4;
	float *data = new float[memSize];
	memset(data, 0, sizeof(float) * memSize);

    m_fbo->Bind();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glViewport(0, 0, m_width, m_height);
    glDisable(GL_DEPTH_TEST);

    fprog->Activate();
    fprog->BindTextures();

    for(int z=0; z<m_depth; z++) {
        // attach texture slice to FBO
        m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[bank], GL_COLOR_ATTACHMENT0, 0, z);
        // render
        drawSlice((z + 0.5f) / (float) m_depth);
    }

	glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, data);

	//memSize = m_width * m_height * m_depth;
	int j = 0;
	for(int i = 0; i < memSize; i+=4)
	{
		m_geomData[j] = data[i+3];
		m_redData[j] = data[i];
		m_greenData[j] = data[i+1];
		m_blueData[j] = data[i+2];
		j++;
	}

    fprog->Deactivate();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    m_fbo->Disable();

	delete[] data;
}*/