#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D frame;
uniform mat4 MVP;

in vec2 uv;

void main() {
	color = vec4(uv, 0., 0.) + texture2D(frame, uv);
}
