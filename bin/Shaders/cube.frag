#version 430 

uniform vec3 p1 = vec3(0.0, 0.0, 0.0);
uniform vec3 p2 = vec3(0.0, 0.0, 0.0);
uniform vec3 color = vec3(0.0, 0.4, 0.4);
uniform float thres = 0.05;
uniform sampler3D tex;

in vec3 TexCoord;

out vec4 fragColor;

int atBoundary(float thres, vec3 coord)
{
    if ( coord.x <= thres || coord.x >= (1.0-thres) ||
            coord.y <= thres || coord.y >= (1.0-thres) ||
            coord.z <= thres || coord.z >= (1.0-thres) ) 
    {
        return 1;
    }
    return 0;
}

void main(void)
{
    if (atBoundary(thres, TexCoord) == 1)
    {
		fragColor = vec4(0.0);
        return;
    }
    vec4 c = texture(tex, TexCoord);
    if (TexCoord.x < p1.x || TexCoord.x > p2.x ||
            TexCoord.y < p1.y || TexCoord.y > p2.y ||
            TexCoord.z < p1.z || TexCoord.z > p2.z) {
        fragColor = c;
		return;
    }
	
    fragColor = vec4(color.x, color.y, color.z, 1.0);
};