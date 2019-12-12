#version 330 core

layout(location = 0) in vec3 position;

uniform sampler2D vertFrame;
uniform vec2 frameSize;

void main() {
	//pass verts strait through
	gl_Position = vec4(position, 1.);
}
