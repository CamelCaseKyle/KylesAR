#version 330 core

layout(location = 0) in vec3 coord;

uniform sampler2D frame;
uniform mat4 MVP;

out vec2 uv;

void main() {
	uv = coord.xy * vec2(.5,-.5) + .5;
	gl_Position = (MVP * vec4(coord.xy, 0., 1.)) * vec4(1.,1.,.1,1.);
}
