// marches an image transformed to have horizontal epipolar lines to compute disparity
#version 330 core

layout(location = 0) out vec3 color;

uniform sampler2D curFrame;
uniform sampler2D wrpFrame;
uniform vec4 frameSize; // pixels x, y, pixel size 1/x, 1/y

in vec2 uv;

const int STEPS = 200; // ~ width / 3
const float rSTEPS = .005, // 1. / STEPS
			eps = .00393;

// simple diff squared matcher
int disparity(in sampler2D iChannel, in vec2 uv, in vec2 px, in vec3 targetFeat) {
	vec2 smp = uv;
	float bestFeat = 9999.;
	int bestI = 0;
	for (int i = 0; i < STEPS; ++i) {
		vec3 featDiff = textureLod(iChannel, smp, 0.).rgb - targetFeat;
		float diffSq = dot(featDiff, featDiff);
		// compare the squared difference
		if (diffSq < bestFeat) {
			bestFeat = diffSq;
			bestI = i;
		}
		smp += px;
	}
	// should have to do with delta transform between two frames instead of steps
	if (bestFeat < eps) return bestI;
	else return 0;
}

void main() {
	vec2 px = vec2(frameSize.z, 0.);
	// the feature we are trying to match
	vec3 f0 = textureLod(curFrame, uv, 0.).rgb;
	// on the warped frame, march until feature is found
	int parallax = max(disparity(wrpFrame, uv, px, f0), disparity(wrpFrame, uv, -px, f0));
	// normalize output
	color = vec3(float(parallax) * rSTEPS);
}
