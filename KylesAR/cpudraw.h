#pragma once

#include <opencv2\core.hpp>
#include "rectExt.h"

//blends RGBA images (move somewhere visible for moar use?)
static void AlphaBlend(const cv::Mat &imgFore, cv::Mat &imgDst, const cv::Point loc = cv::Point(0, 0), const cv::Point pad = cv::Point(0, 0)) {
	if (imgFore.type() != CV_8UC4 || imgDst.type() != CV_8UC3) return;
	//foreground channels rgba, aaa
	std::vector<cv::Mat> foreChannels, foreAlph;
	cv::Mat foreRGB, foreAAA;
	//split rgba
	cv::split(imgFore, foreChannels);
	//spread across channels
	for (int i = 0; i < 3; i++) foreAlph.push_back(foreChannels[3]);
	foreChannels.pop_back();
	//merge
	cv::merge(foreAlph, foreAAA);
	cv::merge(foreChannels, foreRGB);
	//get the ROI
	RectExt BGroi(loc.x, loc.y, foreRGB.cols, foreRGB.rows);
	//enable padding
	BGroi = BGroi.intersect(RectExt(pad.x, pad.y, imgDst.cols - pad.x * 2, imgDst.rows - pad.y * 2));
	RectExt FGroi(cv::Point(0, 0), cv::Point(BGroi.width, BGroi.height));
	//do the blending
	imgDst(BGroi) = foreRGB(FGroi).mul(foreAAA(FGroi), 1. / 255.) + imgDst(BGroi).mul(cv::Scalar::all(255) - foreAAA(FGroi), 1. / 255.);
}
