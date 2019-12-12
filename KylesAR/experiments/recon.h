#pragma once

#include <vector>

#include <opencv2/opencv.hpp>
#include <glm.hpp>

#include "../dev/i_Camera.h"
#include "../ext/i_tracker.h"
//just for printm func
#include "neural.h"

using namespace cv;
using namespace std;

typedef vector<Point2f> Points;
typedef vector<float> Floats;
typedef vector<uchar> UChars;
typedef vector<KeyPoint> Feats;
typedef vector<DMatch> Matches;

// get the bouding polygon of a group of coplanar points
Points QuickHull(Points& cloud);

// print a matrix of uchars (CV_8U)
void printi(cv::Mat& m);

class Recon {
public:
	i_Camera * cam;
	glm::mat4 lastPose;
	bool firstFrame = true;
	// camera frame
	Mat lastFrame, mask, cdesc, ldesc, mdesc, tmpFrame, lastRvec, lastTvec;
	// for optical flow based approach
	Points cpts, lpts;
	Matches m;
	// for feature based approach
	float thresh = 0.002f;
	cv::Ptr<cv::AKAZE> akaze = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_MLDB, 0, 3, thresh, 2, 2, 1);
	cv::Ptr<cv::BFMatcher> bfm = cv::Ptr<cv::BFMatcher>(new cv::BFMatcher(cv::NORM_HAMMING, false));
	Feats cfts, lfts;

	// Ratio of highest to 2nd highest hamming distance to be considered a certain match (0.5 very certain, 0.7 optimal value from the paper, 0.9 very ambiguous)
	float nnMatchRatio = 0.7f;
	// percentage of matches from a cloud to be considered a success
	float matchPercentage = 10.0f;
	// minimum number of matches to care
	int minimumMatches = 50;
	// percentage of inliers during pose recovery to be considered a success
	float inlierPercentage = 30.0f;
	// maximum error in pixels
	float maxReprojError = 0.1f;
	// how far away from the camera we should care about in cm
	float znear = 0.f, zfar = 1000.f;
	// minimum distance for the baseline in cm
	float minDist = 50.f;

	// current step quality
	float quality;

	vector<cv::Point3f> ptCloud;

	Recon() {}

	Recon(i_Camera *icam);

	// pre-process the input stream
	void preProcess(Mat &inp);

	// detect and describe new features (usually faster)
	bool detectAndCompute(Mat &inp, Feats &keypoints, Mat &descriptors);

	// match features between given frames
	bool match(Mat &in1, Mat &in2, Matches &outp);

	// calculate the reprojection error in both views
	float reprojError(PositionObject *curPose, vector<cv::Point3f> out3d);

	// do the 3D triangulation
	void recon(PositionObject* curPose, cv::Mat& out4d);

	// draw debug information
	void debug(PositionObject *curPose);

	// do all the things
	void update(cv::Mat &curFrame, PositionObject *curPose);
};