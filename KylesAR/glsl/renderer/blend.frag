#version 330 core

layout(location = 0) out vec3 color;

uniform int mode;
uniform float warpFactor;
uniform vec2 frameSize;
uniform vec2 depthSize;
uniform vec2 bal;
uniform sampler2D cameraFrame;
uniform sampler2D vertFrame;
//uniform sampler2D depthFrame;

in vec2 tc;

//shadertoy.com/view/Ms2SD1 thanks TDM
float hash(in vec2 p) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}

void main() {
	float offset = float(int(gl_FragCoord.x*frameSize.x > 1.));

	//where to sample textures
	vec2 smp = (gl_FragCoord.xy * frameSize.xy),
		 smp_inv_y = vec2(smp.x, 1.f - smp.y);

	if (mode == 2) {
		//input in screen space [0, 1]
		smp = vec2(tc.x, 1.-tc.y);
		smp_inv_y = tc;
		if (any(lessThan(smp, vec2(0.))) || any(greaterThan(smp, vec2(1.)))) {
			//default to black
			color = vec3(0.);
			return;
		}
	}
		
	vec4 vertCol = texture2D(vertFrame, smp);
	vec3 forecolor = vertCol.rgb;
	vec3 backcolor = vec3(0.);

	//no depth frame
	if (mode == 0) {
		backcolor = vec3(hash(smp));
		vertCol.a = 0.;

	//depth frame normal
	} else if (mode == 1) {
		backcolor = vec3(hash(smp), hash(smp_inv_y), hash(tc));
		vertCol.a = 0.;

	//camera frame
	} else {
		backcolor = texture2D(cameraFrame, smp_inv_y).rgb;
	}

	//final
	color = mix(backcolor, forecolor, vertCol.a);	
}

/*

1 arg
	Invert
		return vec3(1.f - source);		
	Negation
		return vec3((source.g+source.b)*0.5f, (source.r+source.b)*0.5f, (source.r+source.g)*0.5f);
	IDK something
		return vec3(source.g*source.b, source.r*source.b, source.r*source.g);
	Hue
		float fmin = min(min(source.r, source.g), source.b), fmax = max(max(source.r, source.g), source.b);
		return vec3((source - min) * (1.f / (max-min)));
	Saturation
		float fmin = min(min(source.r, source.g), source.b), fmax = max(max(source.r, source.g), source.b), delta = fmax - fmin, lum = (fmax + fmin) / 2.0;
		if (lum < 0.5) return vec3(delta / (fmax + fmin));
		else return vec3(delta / (2.0 - delta));	
	Value
		return vec3(length(source));

2 args
	Add
		return dest + source;
	Subtract
		return dest - source;
	Multiply
		return dest * source;
	color burn
		return 1.f-(1.f-dest) / source;
	linear burn
		return dest + source - 1.f;
	color dodge
		return dest / (1.f - source);
	Screen
		return 1.f - (1.f-dest) * (1.f-source);      
	Average
		return dest + source * 0.5f;
	Exclusion
		return 0.5f - 2.f * (dest - 0.5f)*(source - 0.5f);
		return source*(1.f - dest) * dest*(1.f - source);
	Lighten
		float lumS = dot(source, source), lumD = dot(dest, dest);
		if (lumS > lumD) return source;
		return dest;
	Darken
		float lumS = dot(source, source), lumD = dot(dest, dest);
		if (lumS < lumD) return source;
		return dest;
	Color
		float magS = dot(source, source), magD = dot(dest, dest), magSD = sqrt(magD / magS);
		return source * magSD;
	Luminosity
		float magS = dot(source, source), magD = dot(dest, dest), magDS = sqrt(magS / magD);
		return dest * magDS;
	Desaturate
		float magS = length(source), magD = length(dest);
		return mix(dest, vec3(magD), magS);
	Overlay
		(dest > 0.5) * (1 - (1-2*(dest-0.5)) * (1-source)) + (dest <= 0.5) * ((2*dest) * source)
	Soft Light
		(source > 0.5) * (1 - (1-dest) * (1-(source-0.5))) + (source <= 0.5) * (dest * (source+0.5)) 
	Hard Light  
		(source > 0.5) * (1 - (1-dest) * (1-2*(source-0.5))) + (source <= 0.5) * (dest * (2*source))  
	Vivid Light 
		(source > 0.5) * (1 - (1-dest) / (2*(source-0.5))) + (source <= 0.5) * (dest / (1-2*source))  
	Linear Light 
		(source > 0.5) * (dest + 2*(source-0.5)) + (source <= 0.5) * (dest + 2*source - 1)  
	Pin Light  
		(source > 0.5) * (max(dest,2*(source-0.5))) + (source <= 0.5) * (min(dest,2*source)))  
3+ args
	Bump (vec3 source, float bumpVal, float bumpAng)
		float val = bumpVal - bumpAng, lMag = frac( val );
		if (int(val) & 1) lMag = 1.f - lMag;
		return source*lMag;
   
*/