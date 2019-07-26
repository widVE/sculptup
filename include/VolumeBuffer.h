#ifndef _VOLUME_BUFFER_H
#define _VOLUME_BUFFER_H

#include <GL/glew.h>
#include "framebufferObject.h"
#include "Program.h"

class Shader;

// class to represent a 3D volume (fbo and associated textures)
class VolumeBuffer {
    public:
        VolumeBuffer(GLint internalformat, int width, int height, int depth, int banks, 
                GLenum format=GL_RGBA, GLenum type=GL_FLOAT);
        VolumeBuffer(GLint internalformat, GLuint texid, int width, int height, int depth,
                GLenum format=GL_RGBA, GLenum type=GL_FLOAT);
        ~VolumeBuffer();

        enum BlendMode { BLEND_NONE = 0, BLEND_ADDITIVE };

		void clear(Shader *fprog);

        void bind() { m_fbo->Bind(); }
        void unbind() { m_fbo->Disable(); }

        void setData(unsigned char *data, int bank=0);
        void setData(float *data, int bank=0);

        void setWrapMode(GLint mode, int bank=0);
        void setFiltering(GLint mode, int bank=0);
        void setBlendMode(BlendMode mode) { m_blendMode = mode; }
        void setTexID(GLuint texid, int bank=0);

		void addCube(Shader *fprog, int bank,
			float p1_x, float p1_y, float p1_z,
			float p2_x, float p2_y, float p2_z, float r, float g, float b);

		void erase(Shader *fprog, int bank,
			float p1_x, float p1_y, float p1_z,
			float p2_x, float p2_y, float p2_z,
			float radius, float thresh);

		void fill(Shader *fprog, int bank, float thresh = 0.05f);

		void paint(Shader *fprog, int bank,
			float x, float y, float z,
			float r, float g, float b,
			float radius, float scale);

		void sculpt(Shader *fprog, int bank, float x, float y, float z, float r, float g, float b, float rad, float scale);
        
        //void runProgramReadResults(Program *fprog, int bank=0);

        FramebufferObject *getFBO() { return m_fbo; }
        GLuint getTexture(int bank = 0) { return m_tex[bank]; }

        int getWidth() { return m_width; }
        int getHeight() { return m_height; }
        int getDepth() { return m_depth; }
        int getNumBanks() { return m_banks; }
        GLint getInternalFormat() { return m_internalformat; }
        GLenum getFormat() { return m_format; }
        GLenum getType() { return m_type; }

        float * getData(void) { return m_geomData; }
        float * getRedData(void) { return m_redData; }
        float * getGreenData(void) { return m_greenData; }
        float * getBlueData(void) { return m_blueData; }

		//kevin pull data
		void readResults();

    private:
        GLuint create3dTexture(GLint internalformat, int w, int h, int d, GLenum format=GL_RGBA, GLenum type=GL_FLOAT);
        void drawSlice(float z);

        int m_width, m_height, m_depth;
        int m_max_banks;
        int m_banks;
        BlendMode m_blendMode;
        GLint m_internalformat;
        GLenum m_format;
        GLenum m_type;

        FramebufferObject *m_fbo;
        GLuint *m_tex;

        float *m_geomData;
        float *m_redData;
        float *m_greenData;
        float *m_blueData;

		//kevin
		float *tmpData;
};

#endif
