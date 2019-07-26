#version 430 

uniform float thresh = 0.05;

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
	
    if (atBoundary(thresh, TexCoord) == 1)
    {
        fragColor = vec4(0.0);
    }
	else
	{
		fragColor = vec4(1.0);
	}
};