#version 330 core

layout(location = 0) out float color;

//the depth data
uniform sampler2D depthFrame;
//tracking output
uniform mat4 MVP;

// Input data; will be interpolated for each fragment.
in vec3 uvw;

const float zNear = 0.1, zFar = 1000.0;

float linearDepth(float depthSample) {
    return (2. * zNear) / (zFar + zNear - depthSample * (zFar - zNear));
}

void main() {
	float sceneDepth = texture2D(depthFrame, uvw.xy).r,
		  fragDepth = linearDepth(uvw.z);
	// show them a little range
	color = (fragDepth * 10. - sceneDepth - .5) * 5.;	
}
