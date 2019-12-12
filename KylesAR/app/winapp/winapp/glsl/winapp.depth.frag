#version 330 core

layout(location = 0) out vec4 color;

// Output data; will be interpolated for each fragment.
in vec2 uv_worldspace;
in vec3 vert_worldspace;
in vec3 norm_cameraspace;
in vec3 eye_cameraspace;

//the button texture
uniform sampler2D texture;
uniform sampler2D depthFrame;

uniform vec2 frameSize;

//tracking output
uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 M;
uniform mat4 normalMatrix;

//light tracking output
uniform float light_power;
uniform vec3 light_color;
uniform vec3 light_cameraspace;

float zNear = 0.1, zFar = 1000.0;

float linearDepth(float depthSample) {
    return (2. * zNear) / (zFar + zNear - depthSample * (zFar - zNear));
}

void main() {
	//adjust sample point
	vec2 smp = gl_FragCoord.xy * frameSize.xy;
	//depth sample has been linearized
	float camDepth = texture2D(depthFrame, smp).r;
	//fix linear depth 
	float fragDepth = linearDepth(gl_FragCoord.z) * 2.85 - .31; //also change augment.depth
	//if visible
	if (camDepth < 0.0001 || camDepth > fragDepth)
		color = texture2D(texture, uv_worldspace);
	else
		color = vec4(0.);
}

/*

Shader optimization:
	"Friends don't let friends use Discard"
	MAD multiply then add results in 1 instruction. Most of these are 2x faster than the naive implementation
		x * (1.f - x)	->	x - x * x
		x += a*b + c*d	->	x += a*b; x += c*d;
		(x-start) * slope	->	x * slope + (-start * slope)
		50.f * normalize(vec)	->	vec * (50.f * rsqrt(dot(vec, vec))
		(x-start) / (end - start)	->	x * (1.f/(end-start)) + (-start/(end-start))
		abs() and negate (-) are 'free' on input variables, clamp() is 'free' on output vars
		Use multiplication instead of division (explicite use of rcp() might generate better ASM)
		Group assosiative ops by type (float * float) * (vec * vec) instead of (float * vec) * (float * vec)
		rcp, rsqrt, sqrt, exp2, log2, sin, cos, sincos (not asin acos...) are all hardware instructions on GPU

*/