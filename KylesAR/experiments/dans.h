#pragma once

#include <vector>

#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

const int match_method = TM_SQDIFF_NORMED;
const double akaze_thresh = 2e-3; // AKAZE detection threshold set to locate about 1000 keypoints
const Size subPixWinSize(10, 10), winSize(31, 31);
const int MAX_COUNT = 200;
const double nn_match_ratio = 0.9f; // Nearest-neighbour matching ratio
const double ransac_thresh = 1.f; // RANSAC inlier threshold
const int maxLevel = 3;
const int templateRadius = 31;
const int maxMatches = 100;
const int recalc_freq = 30;
void drawBoundingBox(Mat image, vector<Point2f> bb);
vector<Point2f> toPoints(vector<KeyPoint> keypoints);

class HomographyInfo {
public:
	Mat homography;
	vector<Point2f> inliers1;
	vector<Point2f> inliers2;
	vector<DMatch> inlier_matches;

	HomographyInfo() {}

	HomographyInfo(Mat hom, vector<Point2f> inl1, vector<Point2f> inl2, vector<DMatch> matches);
};

void drawBoundingBox(Mat image, vector<Point2f> bb);

vector<Point2f> toPoints(vector<KeyPoint> keypoints);

//Method for calculating and validating a homography matrix from a set of corresponding points.
HomographyInfo GetHomographyInliers(vector<Point2f> pts1, vector<Point2f> pts2);

int Dans();
