#version 430 

in vec3 tc;
in vec2 pos;

out vec3 tcOut;

void main(void)
{
	tcOut = tc;
	gl_Position = vec4(pos, 0.5, 1.0);
};