#version 430 

uniform vec3 p1 = vec3(0.0, 0.0, 0.0);
uniform vec3 p2 = vec3(0.0, 0.0, 0.0);
uniform float radius = 0.5;
uniform float thresh = 0.05;
//uniform float scale = 1.0;

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

vec3 pointonlineseg(vec3 a, vec3 b, vec3 p)
{
    vec3 c = p-a;
    vec3 v = normalize((b - a)); 
    float d = length(b - a);
    float t = dot(v,c);

    // Check to see if the point is on the line	// if not then return the endpoint	
    if(t < 0) return a;	
    if(t > d) return b;

    // get the distance to move from point a	
    v *= t;

    // move from point a to the nearest point on the segment	
    return a + v;
}

float dist2lineseg(vec3 a, vec3 b, vec3 p)
{
    vec3 e = pointonlineseg(a,b,p);
    return length(p-e);
}

void main(void)
{
    if (atBoundary(thresh, TexCoord) == 1)
    {
		fragColor = vec4(0.0);
        return;
    }
	
    vec4 c = texture(tex, TexCoord);
    float d = dist2lineseg(p1, p2, TexCoord);
    if ( d< radius*0.1)
    {
        float mag = pow(1.0-(d/radius), 16);
        fragColor = clamp(c - vec4(mag), 0.0, 1.0);
    }
    else
        fragColor = c;
};