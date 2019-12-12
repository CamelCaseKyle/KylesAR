#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

out vec2 uv;

uniform sampler2D tex;
uniform vec2 texSize;

void main() {
	//pass texcoords strait through
	uv = texCoord;
	//pass verts strait through
	gl_Position = vec4(position, 1., 1.);
}
