// calculate depth from stereo disparity
// additional constraint of parallax (being signed distnace from registered plane)
#version 330 core

layout(location = 0) in vec3 position;

uniform sampler2D disparity;

out vec2 uv;

void main() {
	
}