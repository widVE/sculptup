#include <gl/glew.h>
#include "Program.h"

#include "nvSDKPath.h"

static nv::SDKPath sdkPath;

Program::Program(CGcontext context, char *filename, char *entry, CGGLenum profileClass)
{
    std::string resolved_path;
    m_profile = cgGLGetLatestProfile(profileClass);

    if (sdkPath.getFilePath( filename, resolved_path)) 
        m_prog = cgCreateProgramFromFile(context, CG_SOURCE, resolved_path.c_str(), m_profile, entry, 0);
    else {
        m_prog = 0;
        fprintf( stderr, "Failed to find shader file '%s'\n", filename);
    }
    cgGLLoadProgram(m_prog);
    scanTexParams();
}

Program::~Program()
{
    cgDestroyProgram(m_prog);   
};

void
Program::Activate()
{
    cgGLBindProgram(m_prog);
    cgGLEnableProfile(m_profile);
}

void
Program::Deactivate()
{
    cgGLDisableProfile(m_profile);
}

void
Program::SetParameter(char *name, float x, float y, float z, float w)
{
    CGparameter param = cgGetNamedParameter(m_prog, name);
    if (param) {
        cgGLSetParameter4f(param, x, y, z, w);
    } else {
        fprintf(stderr, "Error setting parameter '%s'\n", name);
    }
}

// find all texture parameters in program
void
Program::scanTexParams()
{
    CGparameter param = cgGetFirstLeafParameter(m_prog, CG_PROGRAM);
    while (param) 
    {
        CGresource res = cgGetParameterBaseResource(param);
        if (res == CG_TEXUNIT0) 
        {
            TexParam texparam;
            texparam.name = cgGetParameterName(param); 
            texparam.unit = cgGetParameterResource(param) - CG_TEXUNIT0;

            CGtype type = cgGetParameterType(param);
            switch(type) {
            case CG_SAMPLER2D:
                texparam.target = GL_TEXTURE_2D;
                break;
            case CG_SAMPLER3D:
                texparam.target = GL_TEXTURE_3D;
                break;
            case CG_SAMPLERRECT:
                texparam.target = GL_TEXTURE_RECTANGLE;
                break;
            case CG_SAMPLERCUBE:
                texparam.target = GL_TEXTURE_CUBE_MAP;
                break;
            }
            m_texParams.push_back(texparam);
        }
        param = cgGetNextLeafParameter(param);
    }   
}

void
Program::SetTexture(char *name, GLuint tex)
{
    for(unsigned int i=0; i<m_texParams.size(); i++) {
        if (strcmp(name, m_texParams[i].name) == 0) {
            m_texParams[i].tex = tex;
            return;
        }
    }
    fprintf(stderr, "Error setting texture '%s'\n", name);
}

void
Program::BindTextures()
{
    for(unsigned int i=0; i<m_texParams.size(); i++) {
        glActiveTexture(GL_TEXTURE0 + m_texParams[i].unit);
        glBindTexture(m_texParams[i].target, m_texParams[i].tex);       
    }
	glActiveTexture(GL_TEXTURE0);
}