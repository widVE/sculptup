#version 430 

uniform vec3 center = vec3(0.0, 0.0, 0.0);
uniform vec3 color = vec3(0.0, 0.4, 0.4);
uniform float radius = 0.5;
uniform float scale = 1.0;
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
        fragColor =  vec4(0.0);
		return;
    }
    vec4 c = texture(tex, TexCoord);
    float d = (100.0 * scale) * length(TexCoord - center);
    float mag = (((scale * radius)) / pow(d+0.0001, 3.0));
    //c.rgb = color.rgb;
    vec3 cout;

    // TODO color blending not good?
    /*if (c.a > 0.1) {
        cout = (1-mag)*c.rgb + (mag)*color;
    } else {*/
		cout = color;
    //}
	
	//fragColor = vec4(1.0, 1.0, 1.0, 1.0);
    fragColor = clamp(vec4(cout.r, cout.g, cout.b, c.a + mag), 0.0, 1.0);
};