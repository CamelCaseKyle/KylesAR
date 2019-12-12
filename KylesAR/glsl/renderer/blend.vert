#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

uniform int mode;
uniform float warpFactor;
uniform vec2 frameSize;
uniform vec2 depthSize;
uniform vec2 bal;
uniform sampler2D cameraFrame;
uniform sampler2D vertFrame;
//uniform sampler2D depthFrame;

out vec2 tc;

void main() {
	//pass texcoords strait through
	tc = texCoord;
	//pass verts strait through
	gl_Position = vec4(position, 0., 1.f);
}
