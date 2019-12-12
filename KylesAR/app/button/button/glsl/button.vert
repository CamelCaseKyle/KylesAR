#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vert_modelspace;
layout(location = 1) in vec3 norm_modelspace;
layout(location = 2) in vec2 uv_modelspace;

// Output data; will be interpolated for each fragment.
out vec2 uv_worldspace;
out vec3 vert_worldspace;
out vec3 norm_cameraspace;
out vec3 eye_cameraspace;

//the button texture
uniform sampler2D texture;
//screen size
uniform vec2 frameSize;

//tracking output
uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 M;
uniform mat4 normalMatrix;

//light tracking output
uniform float light_power;
uniform vec3 light_color;
uniform vec3 light_cameraspace;

//hover brightness
uniform float brightness;

void main() {
	// Pass through to interpolate
	uv_worldspace = uv_modelspace;

	// Position of the vertex in worldspace
	vert_worldspace = (M * vec4(vert_modelspace,1.)).xyz;
	
	// Normal of the the vertex, in camera space
	norm_cameraspace = (normalMatrix * vec4(norm_modelspace,0.)).xyz;
	
	// Vector that goes from the camera to the vertex, in camera space.
	eye_cameraspace = -(MV * vec4(vert_modelspace,1.)).xyz;
	
	// Output position of the vertex, in clip space : MVP * position
	gl_Position =  MVP * vec4(vert_modelspace, 1.);
}
