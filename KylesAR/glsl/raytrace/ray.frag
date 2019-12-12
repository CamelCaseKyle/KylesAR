#version 330 core

layout(location = 0) out vec4 color;

uniform int mode;
uniform int time;
uniform float fov;
uniform vec2 frameSize;
uniform vec3 camLoc;
uniform mat3 camPose;

const float zNear = 0.1, zFar = 10000., _PI = 3.1415926;

//light location, size
const int mtLight = 1;
const float brightness = 2.5;
vec4 light = vec4(vec3(0., 6., 0.), 1.0);
//returns dist
float map_light(vec3 ro) { return distance(ro, light.xyz) - light.w; }
//we can assume ro is close enough to just normalize
vec3 map_light_norm(vec3 ro) { return normalize(ro - light.xyz); }
//the light should just show up white
vec3 map_light_rgb(vec3 ro, vec3 rd, vec3 nrm) { return vec3(1.0); }

//sphere location, size
const int mtSph = 2;
vec4 sphere = vec4(vec3(0., 2.5, 0.), 5.0);
//returns dist
float map_sph(vec3 ro) { return distance(ro, sphere.xyz) - sphere.w; }
//we can assume ro is close enough to just normalize
vec3 map_sph_norm(vec3 ro) { return normalize(ro - sphere.xyz); }
//the sphere can has epic shading
vec3 map_sph_rgb(vec3 ro, vec3 rd, vec3 nrm) {
    //distane from light
    float pwr = distance(ro, light.xyz);
    //ambience
    pwr = brightness / (1.0 + pwr*pwr);
    //light direction
    vec3 ld = normalize(ro - light.xyz);
    //new reflected direction
    vec3 nd = reflect(ld, nrm);
    //get brightness 
	float moment = 1.0 - clamp(dot(nd, rd), 0., 1.) + pwr;
	float specular = pow(max(0.0,-dot(nd, rd)), 21.0);
	//yeah
	return vec3(0.0, 0.1, 0.3) * moment + specular;
}



vec2 lt(const vec2 l, const vec2 r) { return (l.x < r.x)? l: r; }

//more models will go here
vec3 map_sdf(vec3 ro) {
    vec2 sphres = vec2(map_sph(ro), mtSph),
	     ltres = vec2(map_light(ro), mtLight);
    //light results dont contrib to AO
    float td = max(sphres.x, 0.0);
    return vec3(lt(sphres, ltres), td);
}

//more coloring will go here
vec4 map_rgba(vec3 ro, vec3 rd, vec3 c) {
	vec3 nrm;
    if (int(c.y) == mtSph) {
		//float ao = -0.7 / (1.0 + 10.0*c.z*c.z) + 1.0;
		//get normal
        nrm = map_sph_norm(ro);
		//change color but keep opaque
        return vec4(map_sph_rgb(ro, rd, nrm), 1.0);
    } else if (int(c.y) == mtLight) {
		//get normal
        nrm = map_light_norm(ro);
		//change color but keep opaque
        return vec4(map_light_rgb(ro, rd, nrm), 1.0);
    }
    return vec4(0.0);
}

//marches signed distance fields
vec4 sdfMarch( vec3 ro, vec3 rd, float tmax ) {
    const int steps = 64, lightsteps = 16;
    const float minC = 0.02;
    //start at starting loc
    float t, dt;
    vec3 c, pos = ro;
	//start with transparent black
	vec4 col = vec4(0.);
    //the march loop
    for( int i=0; i<steps; i++ ) {        
        //get min distance
        c = map_sdf(pos);
        //end condition
        if (c.x < minC || t > tmax) break;
        //control step size
        dt = max(c.x, minC);
        //step the ray and update position
        t += dt;
        pos = ro+rd*t;
    }
    //if we hit anything
    if (c.x < minC) col = map_rgba(pos, rd, c);
    //final color
    return col;
}



//plasma properties
float modifier = 0.7,
	  contrast = 20.0,
      clump = 1.0,
      size = 0.0275,
	  ambient = 0.1;

//shadertoy.com/view/Ms2SD1 thanks TDM
float hash(in vec2 p) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}

vec2 csqr( in vec2 a )  { return vec2( a.x*a.x - a.y*a.y, 2.*a.x*a.y  ); }

//plasma noise function
float plasma(vec3 p) {
    float res = 0.;
    vec3 c = p;
    for (int i = 0; i < 3; ++i) {
        p = modifier*abs(p) / dot(p,p) - modifier*clump;
        p.yz = csqr(p.yz);
        res += exp(-contrast * abs(dot(p,c)));
    }
    return res * 0.5;
}
//plasma color
vec3 plasma_rgb( in vec3 pos, in float density ) {
    return normalize(pos + 1.0)*density*density;
}

//marches air space between objects and camera, thanks for inspiration Guil
vec4 densityMarch( in vec3 ro, in vec3 rd, in vec2 tminmax ) {
    const int steps = 128;
    //start at starting loc
    float t = tminmax.x;
    //the ray will get to the end if it hits nothing
    float dt = (tminmax.y - tminmax.x) / float(steps);
    //output color
    vec3 col = vec3(0.);
    //current sample
    float c = 0.;
    for( int i=0; i<steps; i++ ) {
        //this steps through empty space faster
		//we could even flag large voids with negitive densities and use this as a distance function renderer
        t += dt*exp(-c*c);
        vec3 pos = (ro+rd*t)*size;
        //get plasma density
        c = plasma(pos);
		//adjusted sumation, this should be calculated based on 
        col += plasma_rgb(pos, c) * 0.07;
    }
	col = clamp(col, 0.0, 1.0);
    return vec4(col, length(col));
}



void main() {
	
	float offset = float(int(gl_FragCoord.x*frameSize.x > 1.0)),
		  timeoffset = float(time)*_PI*(2.0/255.0);
	color = vec4(0.0);
	
	//convert to normalized space [-1, 1]
	vec2 uv = gl_FragCoord.xy * frameSize.xy * 2.0 - 1.0;
	//recalc everything, uv gets multiplied to match aspect
	vec3 camDir = normalize(vec3(uv * vec2(1.333, -1.0), fov));
	//blarg
	vec3 rayDir = camDir * camPose;

	//density
	if (mode == 0) {
	
		//update fractal (time goes from 0-255)
	    clump = 1.+0.2*sin(timeoffset);
	    size = 0.03-0.01*sin(timeoffset);

		//perform density march
		color = densityMarch(camLoc, rayDir, vec2(10.0, 100.0));

	//sdf
	} else if (mode == 1) {

		//update light
		light.x = cos(timeoffset)*6.0;
	    light.z = sin(timeoffset)*6.0;

		//perform sphere march
		color = sdfMarch(camLoc, rayDir, 100.0);

	}
}
