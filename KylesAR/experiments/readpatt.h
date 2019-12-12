#pragma once

#include <opencv2/opencv.hpp>

#include "../winderps.h"

void readPatt() {
	string pattfile, path = "models\\hiro.patt";
	if (!openTxt(path, pattfile)) return;

	// find first blank line
	int ind = pattfile.find("\n\n");
	if (ind == -1) return;

	// extract first sub-image
	string firstImg = pattfile.substr(0, ind);

	// find how many pixels wide
	ind = firstImg.find_first_of("\n");
	if (ind == -1) return;
	string firstRow = firstImg.substr(0, ind);
	stringstream ss(firstRow);
	string item;
	int imgWidth = 0;
	while (getline(ss, item, ' ')) {
		item = trim(item);
		if (item.length())
			imgWidth++;
	}

	// find how many ints there are
	ss = stringstream(firstImg);
	vector<uchar> results;
	while (getline(ss, item, ' ')) {
		item = trim(item);
		if (item.length()) {
			uchar res = uchar(stoi(item));
			results.push_back(res);
		}
	}

	// there are RGB channels
	int channelLen = results.size() / 3,
		imgHeight = channelLen / imgWidth;

	// create 3 single channel arrays
	uchar vR[256], vG[256], vB[256];
	for (int i = 0; i < channelLen; i++) vR[i] = results[i];
	int ofs = channelLen;
	for (int i = 0; i < channelLen; i++) vG[i] = results[i + ofs];
	ofs = channelLen << 1;
	for (int i = 0; i < channelLen; i++) vB[i] = results[i + ofs];

	// create 3 single channel Mats
	vector<cv::Mat> mRGB = {
		cv::Mat(imgHeight, imgWidth, CV_8UC1, &vR),
		cv::Mat(imgHeight, imgWidth, CV_8UC1, &vG),
		cv::Mat(imgHeight, imgWidth, CV_8UC1, &vB)
	};

	// merge them together into an RGB image
	cv::Mat img, fin;
	cv::merge(mRGB, img);

	fin = cv::Mat(img.rows << 1, img.cols << 1, CV_8UC3, cv::Scalar(0, 0, 0));
	img.copyTo(fin(cv::Rect(img.rows >> 1, img.cols >> 1, img.rows, img.cols)));

	// view results
	cv::imshow("hiro", fin);

}
