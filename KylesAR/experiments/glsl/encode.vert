// marches an image transformed to have horizontal epipolar lines to compute disparity
#version 330 core

layout(location = 0) in vec3 position;

uniform sampler2D curFrame;
uniform mat4 M;
uniform vec4 frameSize; // pixels x, y, pixel size 1/x, 1/y

out vec3 _uv;

void main() {
	//M was calculated for homogeneous NDC coords
	_uv = mat3(M) * vec3(-position.xy, 1.);
	//fix the aspect ratio of the quad to match frame
	gl_Position = vec4(position * vec3(1., frameSize.x / frameSize.y, 1.), 1.);
}
