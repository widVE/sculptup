#version 430

in VertexData
{
	vec4 position;
	vec3 volumeColorPos;
} vertexIn;

uniform vec3 dataStep;
uniform sampler3D dataFieldTex;

const vec3 diffuseMaterial = vec3(0.7, 0.7, 0.7);
const vec3 specularMaterial = vec3(0.99, 0.99, 0.99);
const vec3 ambiantMaterial = vec3(0.1, 0.1, 0.1);

out vec4 fragmentColor;

#define KEVIN_NORMS

void main(void)
{
	//fragmentColor = vec4(1,1,1,1);
	//return;
#if 1
    //vec4 volumeColor = texture(dataFieldTex,volumeColorPos);
    vec3 cleft =	(vertexIn.position.xyz + vec3(dataStep.x, 0, 0)+1.0f)/2.0;
    vec3 cright =	(vertexIn.position.xyz + vec3(-dataStep.x, 0, 0)+1.0f)/2.0;
    vec3 cup =		(vertexIn.position.xyz + vec3(0, dataStep.y, 0)+1.0f)/2.0;
    vec3 cdown =	(vertexIn.position.xyz + vec3(0, -dataStep.y, 0)+1.0f)/2.0;
    vec3 cfront =	(vertexIn.position.xyz + vec3(0, 0, dataStep.z)+1.0f)/2.0;
    vec3 cback =	(vertexIn.position.xyz + vec3(0, 0, -dataStep.z)+1.0f)/2.0;

    vec3 grad = vec3(
		texture(dataFieldTex, cleft).a - texture(dataFieldTex, cright).a, 
		texture(dataFieldTex, cup).a - texture(dataFieldTex, cdown).a, 
    texture(dataFieldTex, cfront).a - texture(dataFieldTex, cback).a);
    
 #ifdef KEVIN_NORMS
//
     float c0 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(-dataStep.x, -dataStep.y, -dataStep.z)+1.0f)/2.0).a;
     float c1 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(dataStep.x, -dataStep.y, -dataStep.z)+1.0f)/2.0).a;
     float c2 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(dataStep.x, -dataStep.y, dataStep.z)+1.0f)/2.0).a;
     float c3 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(-dataStep.x, -dataStep.y, dataStep.z)+1.0f)/2.0).a;
     float c4 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(-dataStep.x, dataStep.y, -dataStep.z)+1.0f)/2.0).a;
     float c5 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(dataStep.x, dataStep.y, -dataStep.z)+1.0f)/2.0).a;
     float c6 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(dataStep.x, dataStep.y, dataStep.z)+1.0f)/2.0).a;
     float c7 =	texture(dataFieldTex,(vertexIn.position.xyz + vec3(-dataStep.x, dataStep.y, dataStep.z)+1.0f)/2.0).a;
     
     grad.x = c0-c1 + c3-c2 + c4-c5 + c7-c6;
     grad.y = c4-c0 + c5-c1 + c6-c2 + c7-c3;
     grad.z = c2-c1 + c3-c0 + c7-c4 + c6-c5;

#endif 

    vec3 volumeColor = (
        texture(dataFieldTex, cleft   ).rgb +
        texture(dataFieldTex, cright  ).rgb +
        texture(dataFieldTex, cup     ).rgb +
        texture(dataFieldTex, cdown   ).rgb +
        texture(dataFieldTex, cfront  ).rgb +
        texture(dataFieldTex, cback   ).rgb ) / 6.0f;

	//volumeColor = 0.5*volumeColor + 0.5*texture(dataFieldTex,volumeColorPos);

    vec3 lightVec=normalize(vec3(0,0,1)-vertexIn.position.xyz);

    vec3 normalVec = normalize(grad);

	vec2 uv = vec2(normalVec.x * 0.5 + 0.5, normalVec.y * 0.5 + 0.5);

    //vec3 color=gl_Color.rgb*0.5+abs(normalVec)*0.5;
    //vec3 color=volumeColor*0.5 + abs(normalVec)*0.5;
    vec3 color=volumeColor;
    //vec3 color = gl_Color.rgb;

    // calculate half angle vector
    vec3 eyeVec = vec3(0.0, 0.0, 1.0);
    vec3 halfVec = normalize(lightVec + eyeVec);

    // calculate diffuse component
    vec3 diffuse = vec3(abs(dot(normalVec, lightVec))) * color*diffuseMaterial;

    // calculate specular component
    vec3 specular = vec3(abs(dot(normalVec, halfVec)));
    specular = pow(specular.x, 32.0) * specularMaterial;

    // combine diffuse and specular contributions and output final vertex color
    fragmentColor.rgb =color.rgb*ambiantMaterial + diffuse + specular;
    fragmentColor.a = 1.0;
#else
    fragmentColor=gl_Color;
#endif


}
