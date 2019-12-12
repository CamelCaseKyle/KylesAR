#version 330 core

layout(location = 0) out vec4 color;

//input data for each fragment
in vec2 uv;
in vec3 hl;
in vec3 sn;
in vec3 rd;
flat in uint mi;

//the textures
uniform sampler2D canTex;
uniform sampler2D metalTex;
uniform sampler2D environment;

//random pose
uniform mat4 NM;
uniform mat4 VP;
uniform mat4 V;

const float PI  = 3.1415926,
			Pi2 = 6.2831852;

//radial azmuth polar
vec2 rap(in vec3 rd) {
	return vec2(atan(rd.z, rd.x) + PI, acos(-rd.y)) / vec2(Pi2, PI);
}

void main() {
	vec3 rfl = reflect(normalize(rd), normalize(sn));
	vec2 rfuv = rap(rfl), _uv = uv;
	float asdf = max(0., pow(1.5 - dot(normalize(rd), normalize(sn)), 3.));
	
	//color = vec3(length(rd) - 20.) * .1;
	//color = vec3(asdf);
	
	_uv.y = 1. - uv.y;
	rfuv.y = 1. - rfuv.y;

	vec4 metal = texture2D(metalTex, _uv.yx);

	if (mi == uint(0)) 
		color = metal;
	else
		color = texture2D(canTex, _uv);
	
	vec4 hdr = texture2D(environment, rfuv);
	hdr += pow(hdr, vec4(15.));
	
	color += hdr * asdf * metal;
}
