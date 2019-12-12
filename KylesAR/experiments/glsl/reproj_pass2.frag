#version 330 core

layout(location = 0) out vec4 color;

uniform sampler2D frame;

in vec2 texCoord;

void main() {
	color = vec4(1.); //texture2D(frame, texCoord);
}
