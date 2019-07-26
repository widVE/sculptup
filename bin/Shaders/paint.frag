#version 430 

uniform vec3 center;
uniform vec3 color = vec3(1.0, 1.0, 1.0);
uniform float radius = 0.5;
uniform float scale = 1.0;

uniform sampler3D tex;

in vec3 TexCoord;

out vec4 fragColor;

void main(void)
{
    vec4 c = texture(tex, TexCoord);
    //kevin -- doubling from 100 to 200
    float d = (200.0 * scale) * length(TexCoord - center);
    float mag = clamp(((scale) / pow(d+0.0001, 3.0)), 0.0, 1.0);
    vec3 cout = (1-mag)*c.rgb + (mag)*color;
    fragColor = vec4(cout, c.a);
};