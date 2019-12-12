// marches an image transformed to have horizontal epipolar lines to compute disparity
#version 330 core

layout(location = 0) out vec3 color;

uniform sampler2D curFrame;
uniform mat4 M;
uniform vec4 frameSize; // pixels x, y, pixel size 1/x, 1/y

in vec3 _uv;

void main() {
	//perspective texture divide
	vec2 uv = (_uv.xy / _uv.z) * .25 + .5;
	//map OB pixels to black
	if (any(greaterThan(uv, vec2(1.))) || any(lessThan(uv, vec2(0.)))) color = vec3(0.);
	//mipmap for SAD kernel size (16?)
	else color = textureLod(curFrame, uv, 0.).rgb;
}

// consider y = 4. * pow(x - .5, 3.) + .5