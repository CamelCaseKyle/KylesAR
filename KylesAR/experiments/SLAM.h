#pragma once

#include <vector>

#include <opencv2/opencv.hpp>

#include "../dev/i_Camera.h"
#include "../ext/i_tracker.h" // not important
#include "../dsp.h" //optional

#define LOG_POSE

using namespace std;
using namespace cv;

typedef vector<Point2f> Points;
typedef vector<float> Floats;
typedef vector<uchar> UChars;
typedef vector<KeyPoint> Feats;
typedef vector<DMatch> Matches;

class SLAM {
public:
	//need the camera
	i_Camera* cam;
	//additional calibration
	double flen = 1., trajScale = 3.;
	Point2d pp = Point2d(0., 0.);
	//camera frame
	Mat cf, lf, traj, mask, imgPts;
	//for optical flow based approach
	Points cpts, lpts;
	Floats qpts;
	UChars ptstat, femstat;
	//for feature based approach
	cv::Ptr<cv::AKAZE> akaze = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_MLDB, 0, 3, .0002, 1, 4, 1);
	cv::Ptr<cv::BFMatcher> bfm = cv::Ptr<cv::BFMatcher>(new cv::BFMatcher(cv::NORM_L2, false));
	Feats cfts, lfts;
	Mat cdesc, ldesc;
	//output
	PositionObject pose = PositionObject("SLAM");
	//last point for drawing traj
	Point lpt;
	//pose filter and prediction
	sgFilter fltr = sgFilter();

	// create the instance
	SLAM(i_Camera *inpCam);
	// calculate OpenGl matrix
	void getMatrix(PositionObject &pose);
	// detect and describe feature points
	void detect(Mat &inp, Feats &inout1, Mat &out1);
	// match feature points between frames
	void match(Mat &in1, Mat &in2, Feats &f1, Feats &f2, Matches &outp);
	// calculate the reprojection error to help validate pose
	float reprojError(PositionObject *curPose, PositionObject *lastPose, vector<cv::Point3f> out3d);
	// pre-process the frame
	void processFrame(Mat &inp, Mat &outp);
	// show the features and trajectory
	void show();
	// main function
	void update();
};
