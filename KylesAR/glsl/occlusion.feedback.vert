#version 330 core

//the fullscreen quad
layout(location = 0) in vec2 content_uv;
//verts of the object in object space
layout(location = 1) in vec3 content_vert;

//the depth data
uniform sampler2D depthFrame;
//tracking output
uniform mat4 MVP;

// Output data; will be interpolated for each fragment.
out vec3 uvw;

void main() {
	//need to perspective divide?
	vec4 tv = MVP * vec4(content_vert, 1.);
	//to clip space [0. - 1.]
	uvw = (tv.xyz / tv.w) * vec3(.5, .5, 1.) + vec3(.5, .5, 0.);
	// Pass through to interpolate
	gl_Position = vec4(content_uv * 2. - 1., 0., 1.);
}