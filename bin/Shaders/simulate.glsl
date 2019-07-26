#version 430 

// test if coord at volume boundary
int atBoundary(float thres, vec3 coord)
{
    if ( coord[0] <= thres || coord[0] >= (1-thres) ||
            coord[1] <= thres || coord[1] >= (1-thres) ||
            coord[2] <= thres || coord[2] >= (1-thres) ) 
    {
        return 1;
    }
    return 0;
}

vec4 clear(sampler3D x)
{
    return vec4(0.5, 0.5, 0.5, 0.0);
}

vec4 copy(vec3 x,
        uniform sampler3D tex
        )
{
    return texture3D(tex, x);
}

vec4 fill(vec3 x,
        uniform float thres = 0.05
        )
{
    if (atBoundary(thres, x) == 1)
    {
        return vec4(0.0);
    }
    return vec4(1.0);
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


// render a splat with smooth falloff
vec4 splat(vec3 x,
        uniform vec3 center,
        uniform sampler3D tex,
        uniform vec3 color = vec3(0.0f, 0.4f, 0.4f),
        uniform float radius = 0.5,
        uniform float thres = 0.05,
        uniform float scale = 1.0
        )
{
    if (atBoundary(thres, x) == 1)
    {
        return vec4(0.0);
    }
    vec4 c = texture3D(tex, x);
    float d = (100.0 * scale) * length(x - center);
    float mag = (((scale * radius)) / pow(d+0.0001, 3.0));
    //c.rgb = color.rgb;
    vec3 cout;

    // TODO color blending not good?
    if (c.a > 0.1) {
        cout = (1-mag)*c.rgb + (mag)*color;
    } else {
		cout = color;
    }

    return clamp(vec4(cout.r, cout.g, cout.b, c.a + mag), 0.0, 1.0);//c + vec4(mag), 0.0, 1.0); 
}

// render a splat with smooth falloff
vec4 splatp2p(vec3 x : TEXCOORD0,
        uniform vec3 p1,
        uniform vec3 p2,
        uniform sampler3D tex,
        uniform vec3 color1 = vec3(0.0f, 0.4f, 0.4f),
        uniform vec3 color2 = vec3(0.0f, 0.4f, 0.4f),
        uniform float radius1 = 0.5,
        uniform float radius2 = 0.5,
        uniform float thres = 0.05,
        uniform float scale = 1.0,
        uniform bool interpRadius = True,
        uniform bool interpColor = True,
        uniform float interpFalloff = 1.0
        ) : COLOR
{
    if (atBoundary(thres, x) == 1) {
        return vec4(0.0);
    }

    vec4 c = texture3D(tex, x);

    // get line, is on?
    vec3 coord = x-p1;
    vec3 v = normalize((p2 - p1)); 
    float dist = length(p2 - p1);
    float t = dot(v,coord);

    // Check to see if the point is on the line
    // if not, return
    if(t < 0) return c;	
    if(t > dist) return c;

    // linear interp mag and color between points
    float delta = 0.0001;
    float pow_exp = 4.0;
    float scalar = 100.0 * scale;
    float d1, d2, mag1, mag2;

    d1 = scalar * length(x - p1);
    d2 = scalar * length(x - p2);

    // solid fill or gradient
    if (interpRadius) {
        mag1 = ((scale * radius1) / pow(d1+delta, pow_exp));
        mag2 = ((scale * radius2) / pow(d2+delta, pow_exp));
    } else {
        float rad = scale * radius1;
        mag1 = (rad / pow(d1+delta, pow_exp));
        mag2 = (rad / pow(d2+delta, pow_exp));
    }

    float norm = t/dist;
    float falloff = 1.0 / pow(dist+delta, pow_exp);
    float mag = (1-norm)*mag1 + falloff*norm*mag2;
    vec3 colorinterp = (1-norm)*color1 + falloff*norm*color2;

    vec3 cout;
    if (c.a > 0.1) {
        cout = (1-mag)*c.rgb + (mag)*colorinterp;
    } else {
        cout = colorinterp;
    }

    return clamp(vec4(cout.r, cout.g, cout.b, c.a + mag), 0.0, 1.0);
}

vec4 cube(vec3 x,
        uniform sampler3D tex,
        uniform vec3 p1,
        uniform vec3 p2,
        uniform float thres = 0.05,
        uniform vec3 color = vec3(1.0f, 0.0f, 1.0f)
        ) : COLOR
{
    if (atBoundary(thres, x) == 1)
    {
        return vec4(0.0);
    }
    vec4 c = texture3D(tex, x);
    if (x.x < p1.x || x.x > p2.x ||
            x.y < p1.y || x.y > p2.y ||
            x.z < p1.z || x.z > p2.z) {
        return c;
    }
    return vec4(color.x, color.y, color.z, 1.0);
}

vec4 erase(vec3 x : TEXCOORD0,
        uniform sampler3D tex,
        uniform vec3 p1,
        uniform vec3 p2,
        uniform float radius = 0.5,
        uniform float thres = 0.05,
        uniform float scale = 1.0,
        uniform int mode = 0
        ) : COLOR
{
    if (atBoundary(thres, x) == 1)
    {
        return vec4(0.0);
    }
    vec4 c = texture3D(tex, x);
    float d = dist2lineseg(p1, p2, x);
    if ( d< radius*0.1)
    {
        float mag = pow(1.0-(d/radius),16);
        return clamp(c - vec4(mag), 0.0, 1.0);

    }
    else
        return c;
}

vec4 paint(vec3 x,
        uniform vec3 center,
        uniform vec3 color,
        uniform sampler3D tex,
        uniform float radius = 0.5,
        uniform float scale = 1.0
        )
{
    vec4 c = texture3D(tex, x);
    //kevin -- doubling from 100 to 200
    float d = (200.0 * scale) * length(x - center);
    float mag = clamp(((scale) / pow(d+0.0001, 3.0)), 0.0, 1.0);
    vec3 cout = (1-mag)*c.rgb + (mag)*color;
    return vec4(cout, c.a);
}

vec4 indicator(vec3 x,
        uniform vec3 center,
        uniform vec3 color,
        uniform sampler3D tex,
        uniform float radius = 0.5,
        uniform float scale = 1.0)
{
    vec4 c = texture3D(tex, x);
    return c;
}

void main(void)
{

};