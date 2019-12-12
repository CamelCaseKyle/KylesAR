#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D depthFrame;
uniform sampler2D depthCalib;
uniform vec2 depthSize;
uniform float aspect;
uniform float scale;
uniform mat4 PFD;

in float depth;

void main() {
	color = vec4(depth,depth,depth, 0.);
}
