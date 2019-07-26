#version 430

layout(location = 0) in vec3 vertexPosition;

out VS_OUT
{
	vec4 position;
} gs_in;

uniform mat4 mvp;

void main(void)
{
	gs_in.position = vec4(vertexPosition, 1.0);
	gl_Position = mvp * vec4(vertexPosition, 1.0);
}
