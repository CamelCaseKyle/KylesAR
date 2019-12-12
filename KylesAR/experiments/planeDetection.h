#pragma once

#include "../render.h"

class PlaneDetection {
public:
	cv::Mat nrml;
	Content pd_cont;
	Context *pdrend;
	string pdcwd;
	Shader normShade;

	//plane detection setup
	PlaneDetection(Context &rend);
	//plane detection
	void update(cv::Point _p);
	//normal map render
	void render();
};
