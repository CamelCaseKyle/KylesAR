#version 330 core

layout(location = 0) in vec3 uv;

uniform sampler2D depthFrame;
uniform sampler2D depthCalib;
uniform vec2 depthSize;
uniform float aspect;
uniform float scale;
uniform mat4 PFD;

out float depth;

void main() {
	//texture is upside down
	vec2 smp = uv.xy * vec2(.5, -.5) + .5;
	//depth offset calibration
    float val = texture2D(depthFrame, smp).r * scale * (1. - .05*(texture2D(depthCalib, smp).r-.5));
	//uv is a ray that projects, value (distance to image plane) into worldspace
	vec4 vertex = vec4(uv * vec3(1., aspect, 1.) * val, 1.),
		 //transform point to align with RGB
		 newVertex = PFD * vertex;
	//outputs
	depth = newVertex.z * .01 - .5;
	gl_Position = newVertex;
	gl_PointSize = newVertex.z * .05 + 1.;
}
