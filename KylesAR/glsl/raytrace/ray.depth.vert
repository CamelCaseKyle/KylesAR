#version 330 core
layout(location = 0) in vec3 position;

uniform sampler2D depthFrame;

uniform int mode;
uniform int time;
uniform float fov;
uniform vec2 frameSize;
uniform vec3 camLoc;
uniform mat3 camPose;

void main() {
	//pass verts strait through
	gl_Position = vec4(position, 1.f);
}
