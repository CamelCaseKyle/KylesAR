#pragma once

#include "../render.h"

class ParallaxSFM {
public:
	//circular buffer of textures taken every .25 seconds or after so much movement
	//save encoded buffers, warp them 
	cv::Mat lastFrame;
	glm::mat4 lastPose;
	bool firstFrame = true;
	Content paraCont;
	Context *paraRend;
	Shader disparityShade, encodeShade;

	ParallaxSFM(Context *rend);

	void compute(cv::Mat curFrame, glm::mat4 curPose);
};

