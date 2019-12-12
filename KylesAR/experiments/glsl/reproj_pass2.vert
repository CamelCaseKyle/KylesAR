#version 330 core

layout(location = 0) in vec3 coord;
layout(location = 1) in vec2 uv;

uniform sampler2D frame;

out vec2 texCoord;

void main() {
	texCoord = uv;
	gl_Position = vec4(coord, 1.);
}
