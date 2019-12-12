#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vert;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec2 tex;
layout(location = 3) in uint matind;

// Output data; will be interpolated for each fragment.
out vec2 uv;
out vec3 hl;
out vec3 sn;
out vec3 rd;
flat out uint mi;

//the textures
uniform sampler2D canTex;
uniform sampler2D metalTex;
uniform sampler2D environment;

//random pose
uniform mat4 NM;
uniform mat4 VP;
uniform mat4 V;

void main() {
	//fragment texture coord
	uv = tex;
	//hit location of the fragment
	hl = vert;
	//normal of surface
	sn = norm;
	//ray direction from the camera to the fragment
	rd = -(V * vec4(vert, 1.)).xyz;
	//material index
	mi = matind;
	//output position of the vertex in clip space
	gl_Position = VP * vec4(vert, 1.);
}
