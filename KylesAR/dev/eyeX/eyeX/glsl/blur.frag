#version 330 core
layout(location = 0) out vec3 color;

in highp vec2 blurCoordinates[5];

uniform sampler2D inputImageTexture;
uniform sampler2D focusImageTexture;

uniform highp float texelWidthOffset;
uniform highp float texelHeightOffset;

void main() {
	lowp vec3 sum = vec3(0.0);
	
	float smpl = texture2D(focusImageTexture, blurCoordinates[0]).r;

	vec2 d1 = (blurCoordinates[1] - blurCoordinates[0]) * smpl,
		 d2 = (blurCoordinates[2] - blurCoordinates[0]) * smpl,
		 d3 = (blurCoordinates[3] - blurCoordinates[0]) * smpl,
		 d4 = (blurCoordinates[4] - blurCoordinates[0]) * smpl;
																  
	sum += texture2D(inputImageTexture, blurCoordinates[0]   ).rgb * 0.204164f;
	sum += texture2D(inputImageTexture, blurCoordinates[0]+d1).rgb * 0.304005f;
	sum += texture2D(inputImageTexture, blurCoordinates[0]+d2).rgb * 0.304005f;
	sum += texture2D(inputImageTexture, blurCoordinates[0]+d3).rgb * 0.093913f;
	sum += texture2D(inputImageTexture, blurCoordinates[0]+d4).rgb * 0.093913f;
	
	color = sum;
}
