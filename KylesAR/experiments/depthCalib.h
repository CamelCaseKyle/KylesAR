// point the depth camera at some planes known distances away
// generate a projection space voxel grid of offsets
// fix depth data

#pragma once

#include <opencv2\opencv.hpp>

class DepthCalib {
public:
	//monte carlo depth calib setup
	cv::Mat montecarlo = cv::Mat::zeros(480, 640, CV_32FC1);
	bool dcIntegrate = false;
	int fnum = 0;

	DepthCalib() {}

	//average in a frame
	void update(cv::Mat frame);
	//render it
	void render();
};
