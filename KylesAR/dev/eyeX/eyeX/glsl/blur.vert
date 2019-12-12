#version 330 core
layout(location = 0) in vec3 position;

out highp vec2 blurCoordinates[5];

uniform sampler2D inputImageTexture;
uniform sampler2D focusImageTexture;

uniform float texelWidthOffset;
uniform float texelHeightOffset;
 
void main() {
	//pass verts strait through
	gl_Position = vec4(position, 1.f);

	//mostly a compute shader for the blur weights
	vec3 inputTextureCoordinate = vec3((position.xy+vec2(1,1))*0.5f, 0.f);
	vec2 singleStepOffset = vec2(texelWidthOffset, texelHeightOffset);

	blurCoordinates[0] = inputTextureCoordinate.xy;
	blurCoordinates[1] = inputTextureCoordinate.xy + singleStepOffset * 1.407333;
	blurCoordinates[2] = inputTextureCoordinate.xy - singleStepOffset * 1.407333;
	blurCoordinates[3] = inputTextureCoordinate.xy + singleStepOffset * 3.294215;
	blurCoordinates[4] = inputTextureCoordinate.xy - singleStepOffset * 3.294215;
}
