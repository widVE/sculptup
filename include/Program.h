#ifndef _CG_PROGRAM_H
#define _CG_PROGRAM_H

#include <vector>
#include <map>
#include <Cg/cgGL.h>

class Program {
public:
    Program(CGcontext context, char *filename, char *entry, CGGLenum profileClass);
    ~Program();

    void Activate();
    void Deactivate();

    void SetParameter(char *name, float x, float y=0.0, float z=0.0, float w=0.0);
    void SetTexture(char *name, GLuint tex);

    void BindTextures();

private:
    struct TexParam {
        const char *name;
        int unit;
        int target;
        int tex;
    };

    void scanTexParams();

    CGprogram m_prog;
    CGprofile m_profile;
    std::vector<TexParam> m_texParams;
};

#endif
