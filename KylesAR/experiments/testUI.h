#pragma once

#include <opencv2/opencv.hpp>

#include "../kibblesmath.h"

using namespace cv;
using namespace std;

void testUI(Mat &frame) {
	//~60fps
	while (waitKey(15) == -1) {
		//get frame for working
		vector<Mat> chan;
		Mat curframe, curHSV, tmp, tmp2zero, tmp2zeroinv;

		if (frame.type() == CV_8UC4) cvtColor(frame, curframe, CV_BGRA2BGR);
		else if (frame.type() == CV_8UC3) frame.copyTo(curframe);
		else return;

		RectExt roi(20, 20, 200, 50);
		int s = curframe.rows >> 3;

		//cheap box blur value channel
		blur(curframe, tmp, Size(s, s));
		//to hsv
		cvtColor(tmp, curHSV, CV_BGR2HSV);
		split(curHSV, chan);
		//60 degree shift
		add(chan[0], Scalar(30), chan[0]);
		//wrap out-of-bound values back
		threshold(chan[0], tmp2zero, 180., 255., THRESH_TOZERO);
		threshold(chan[0], tmp2zeroinv, 180., 255., THRESH_TOZERO_INV);
		add(tmp2zero, tmp2zeroinv, chan[0]);
		//average sat val for val
		addWeighted(chan[1], .5, chan[2], .5, 0., chan[2]);
		//dont max out val
		chan[2] = Mat(chan[2].rows, chan[2].cols, chan[2].type(), Scalar(98));
		//opencv lut refine?

		//put back together
		merge(chan, curHSV);
		tmp = cv::Mat::zeros(tmp.size(), tmp.type());

		//draw UI
		cvtColor(curHSV(roi), tmp(roi), CV_HSV2BGR);
		putText(tmp, "UI element", Point(30, 60), CV_FONT_HERSHEY_PLAIN, 2., Scalar(255, 255, 255), 2);

		//best function ever
		addWeighted(curframe, .8, tmp, .5, 0., curframe);

		cv::imshow("testUI", curframe);
	}
	destroyAllWindows();
}