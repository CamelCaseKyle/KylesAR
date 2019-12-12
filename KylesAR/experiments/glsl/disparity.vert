// marches an image transformed to have horizontal epipolar lines to compute disparity
#version 330 core

layout(location = 0) in vec3 position;

uniform sampler2D curFrame;
uniform sampler2D wrpFrame;
uniform vec4 frameSize; // pixels x, y, pixel size 1/x, 1/y

out vec2 uv;

void main() {
	uv = position.xy * .5 + .5;
	gl_Position = vec4(position, 1.);
}
