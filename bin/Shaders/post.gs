#version 330 core

//uniform sampler2D ColorBuffer;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform float zSlice = 0.f;

out vec3 TexCoord;

void main() {
	
	gl_Position = vec4( 1.0, 1.0, 0.5, 1.0 );
	TexCoord = vec3( 1.0, 1.0, zSlice );
	EmitVertex();

	gl_Position = vec4(-1.0, 1.0, 0.5, 1.0 );
	TexCoord = vec3( 0.0, 1.0, zSlice ); 
	EmitVertex();

	gl_Position = vec4( 1.0,-1.0, 0.5, 1.0 );
	TexCoord = vec3( 1.0, 0.0, zSlice ); 
	EmitVertex();

	gl_Position = vec4(-1.0,-1.0, 0.5, 1.0 );
	TexCoord = vec3( 0.0, 0.0, zSlice ); 
	EmitVertex();

	EndPrimitive(); 
}
