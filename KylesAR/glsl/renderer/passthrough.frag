#version 330 core

layout(location = 0) out vec3 color;

in vec2 uv;

uniform sampler2D tex;
uniform vec2 texSize;

void main() {
	color = texture2D(tex, uv).rgb;
}
