#pragma once

#include <vector>

#include <opencv2/opencv.hpp>

#include "../winderps.h"
#include "../dev/i_Camera.h"

using namespace std;
using namespace cv;

typedef vector<Point2f> Points;
typedef vector<float> Floats;
typedef vector<uchar> UChars;
typedef vector<KeyPoint> Feats;
typedef vector<DMatch> Matches;

class KylesNFT {
public:
	i_Camera * cam;
	Mat mask, cdesc, fdesc, tmpFrame, rvec, tvec, lastRvec, lastTvec;
	Points cpts, ucpts, reprojpts;
	std::vector<int> inliers;
	std::vector<cv::Point3f> fpts, ftCloud;
	Matches m;
	//for feature based approach
	cv::Ptr<cv::AKAZE> akaze = cv::AKAZE::create(cv::AKAZE::DESCRIPTOR_MLDB, 0, 3, thresh, 2, 2, 1);
	cv::Ptr<cv::BFMatcher> bfm = cv::Ptr<cv::BFMatcher>(new cv::BFMatcher(cv::NORM_HAMMING, false));
	Feats cfts;
	// feature significance threshold (auto optimized)
	float thresh = 0.01f;
	// Ratio of highest to 2nd highest hamming distance to be considered a certain match (auto optimized)
	float nnMatchRatio = 0.7f;
	// percentage of matches from a cloud to be considered a success
	float matchPercentage = 20.0f;
	// minimum number of matches to care
	int targetKeypoints = 200;
	// percentage of inliers during pose recovery to be considered a success
	float inlierPercentage = 20.0f;
	// maximum error in pixels
	float maxReprojError = 1.0f;
	// how far away from the object we should care about
	float znear, zfar;
	// current step quality
	float quality;
	uint frame = 0, wait = 0, step = 0;
	bool firstPose = true;

	//setup tracker with camera
	KylesNFT(i_Camera* icam);
	//load feature cloud from disk
	bool loadFtCloud(string path);
	//pre-process the input stream
	void preProcess(Mat &inp);
	//detect and describe new features (usually faster)
	bool detectAndCompute(Mat &inp, Feats &keypoints, Mat &descriptors);
	//match features between given frames
	bool match(Mat &in1, Mat &in2, Matches &outp);
	//calc reprojection error in one view
	float reprojError(std::vector<int>& inliers);
	//do the PNP
	bool getMatrix();
	//draw debug information
	void debug();
	//do all the things
	void update(cv::Mat &curFrame);
};