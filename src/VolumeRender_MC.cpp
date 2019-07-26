#include "VolumeRender_MC.h"

#include "GL/glew.h"

#include "nvSDKPath.h"
#include "GLUT/glut.h"

#include "VolumeBuffer.h"

static nv::SDKPath sdkPath;

VolumeRender_MC::VolumeRender_MC(CGcontext cg_context, VolumeBuffer *volume)
	: VolumeRender(cg_context,volume)
{
}

VolumeRender_MC::~VolumeRender_MC() 
{
	VolumeRender::~VolumeRender();
}

void VolumeRender_MC::loadPrograms()
{
    m_cg_vprofile = cgGLGetLatestProfile(CG_GL_VERTEX);
    m_cg_fprofile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

    std::string resolved_path;

    if (sdkPath.getFilePath( "shaders/raymarch.cg", resolved_path)) {
        m_raymarch_vprog = cgCreateProgramFromFile( m_cg_context, CG_SOURCE, resolved_path.c_str(), m_cg_vprofile , "RayMarchVP", 0);
        cgGLLoadProgram(m_raymarch_vprog);

        m_raymarch_fprog = cgCreateProgramFromFile( m_cg_context, CG_SOURCE, resolved_path.c_str(), m_cg_fprofile , "RayMarchFP", 0);
        cgGLLoadProgram(m_raymarch_fprog);

        m_density_param = cgGetNamedParameter(m_raymarch_fprog, "density");
        m_brightness_param = cgGetNamedParameter(m_raymarch_fprog, "brightness");
    }
    else {
        fprintf( stderr, "Failed to find shader file '%s'\n", "src/render_texture_3D/shaders/raymarch.cg");
    }
}

void VolumeRender_MC::render() {
    cgGLBindProgram(m_raymarch_vprog);
    cgGLEnableProfile(m_cg_vprofile);

    cgGLEnableProfile(m_cg_fprofile);

    cgGLBindProgram(m_raymarch_fprog);
    cgGLSetParameter1f(m_density_param, m_density);
    cgGLSetParameter1f(m_brightness_param, m_brightness);

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_3D, m_volume->getTexture());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glutSolidCube(2.0);

    cgGLDisableProfile(m_cg_vprofile);
    cgGLDisableProfile(m_cg_fprofile);
}