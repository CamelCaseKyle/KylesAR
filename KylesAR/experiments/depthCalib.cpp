#include "depthCalib.h"

//average in a frame
void DepthCalib::update(cv::Mat frame) {
	if (dcIntegrate) {
		cv::Mat tmp;
		frame.convertTo(tmp, CV_32FC1, 1.f / 65535.f);
		montecarlo += tmp;
		fnum++;
	} else {
		frame.convertTo(montecarlo, CV_32FC1, 1.f / 65535.f);
	}
}

//render it
void DepthCalib::render() {
	if (dcIntegrate) {
		cv::imshow("depth calib", 10. * montecarlo / float(fnum));
	} else {
		cv::imshow("depth calib", 10. * montecarlo);
	}
	if (cv::waitKey(1) == 'd') {
		dcIntegrate = true;
		montecarlo = cv::Mat::zeros(480, 640, CV_32FC1);
	}
}
