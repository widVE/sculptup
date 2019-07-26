#ifndef VOLUMERENDER
#define VOLUMERENDER

#include <gl/glew.h>
#include <Cg/cgGL.h>

class VolumeBuffer;
// class to render a 3D volume
class VolumeRender  {
public:
    VolumeRender(CGcontext cg_context, VolumeBuffer *volume);
    ~VolumeRender();

    virtual void render();

    void setVolume(VolumeBuffer *volume) { m_volume = volume; }

    void setDensity(float x) { m_density = x; }
    void setBrightness(float x) { m_brightness = x; }

protected:
    virtual void loadPrograms();

    VolumeBuffer *m_volume;

    CGcontext m_cg_context;
    CGprofile m_cg_vprofile, m_cg_fprofile;

    CGprogram m_raymarch_vprog, m_raymarch_fprog;
    CGparameter m_density_param, m_brightness_param;

    float m_density, m_brightness;
};
#endif