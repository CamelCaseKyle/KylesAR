#include "knft.h"

KylesNFT::KylesNFT(i_Camera* icam) : cam(icam) {
		rvec = Mat::eye(3, 3, CV_64FC1);
		tvec = Mat::zeros(3, 1, CV_64FC1);
	}

bool KylesNFT::loadFtCloud(string path) {
	// get configuration
	std::vector<std::string> lines;
	if (!openTxt(path, lines)) return false;

	// parse config
	string header = lines[0];

	// header should look something like: "ptcloud 79 MLDB 0 3 0.005 2 2 1\n"
	vector<string> fields = split(header, " ");
	int npts = std::stoi(fields[1]);
	for (int i = 1; i <= npts; i++) {
		fields = split(lines[i], " ");
		float x = std::stof(fields[0]),
			  y = std::stof(fields[1]),
			  z = std::stof(fields[2]);
		ftCloud.push_back(cv::Point3f(x, y, z));
	}

	// now for the descriptors
	int end = npts * 2, width = split(lines[npts + 1], " ").size();
	cv::Mat new_fdesc = cv::Mat::zeros(npts, width, CV_8UC1);
	for (int i = npts + 1; i < lines.size(); i++) {
		fields = split(lines[i], " ");
		for (int j = 0; j < width; j++) {
			int num = std::stoi(fields[j]);
			new_fdesc.at<uchar>(i - npts - 1, j) = num;
		}
	}

	// check if fdesc exists
	if (fdesc.empty()) {
		fdesc = new_fdesc;
	} else {
		// merge with fdesc
		std::vector<cv::Mat> mats = { fdesc, new_fdesc };
		cv::vconcat(mats, fdesc);
	}
}

//pre-process the input stream
void KylesNFT::preProcess(Mat &inp) {
	// use full size full color but maybe chop it up into smaller images
	inp.copyTo(tmpFrame);
	//resize(inp, tmpFrame, cv::Size(), 0.5, 0.5);
}

//detect and describe new features (usually faster)
bool KylesNFT::detectAndCompute(Mat &inp, Feats &keypoints, Mat &descriptors) {

	akaze->detectAndCompute(inp, mask, keypoints, descriptors);
	//printf("compute_detect %i at %f\n", descriptors.rows, thresh);

	float delta = float(descriptors.rows - targetKeypoints) * 0.000333f;
	thresh += delta * delta * glm::sign(delta);
	thresh = std::min(std::max(thresh, 0.0001f), 0.1f);
	akaze->setThreshold(thresh);

	return keypoints.size() > 4;
}

//match features between given frames
bool KylesNFT::match(Mat &in1, Mat &in2, Matches &outp) {

	vector<Matches> tmp;
	bfm->knnMatch(in1, in2, tmp, 2);

	outp.clear();
	//Do ratio test to remove outliers from KnnMatch
	for (int i = 0; i < tmp.size(); i++) {
		//Ratio Test for outlier removal, removes ambiguous matches.
		if (tmp[i][0].distance < nnMatchRatio * tmp[i][1].distance) {
			outp.push_back(tmp[i][0]);
		}
	}

	quality = 100.f * float(outp.size()) / float(in1.rows);
	//printf("match inputs: %i %i output: %i (%f%%)\n", in1.rows, in2.rows, outp.size(), quality);

	nnMatchRatio += (quality < matchPercentage) ? 0.01f : -0.01f;
	nnMatchRatio = std::min(std::max(nnMatchRatio, 0.5f), 0.95f);

	return m.size() > 4 && quality > matchPercentage;
}

//calc reprojection error in one view
float KylesNFT::reprojError(std::vector<int>& inliers) {
	cv::projectPoints(fpts, rvec, tvec, cam->rgb.camMat, noArray(), reprojpts);

	float error = 0.f;
	for (int i = 0; i < inliers.size(); i++) {
		int ind = inliers[i];
		Point2f d = reprojpts[ind] - ucpts[ind];
		error += d.dot(d);
	}
	error /= float(inliers.size());
	return glm::sqrt(error);
}

//do the PNP
bool KylesNFT::getMatrix() {
	// initial estimate with inliers pass cv::noArray() for distortion
	solvePnPRansac(fpts, ucpts, cam->rgb.camMat, cv::noArray(), rvec, tvec, false, 500, 1.0f, 0.98f, inliers);

	float quality = 100.f * float(inliers.size()) / float(ucpts.size());
	//printf("inliers %i %i (%f\%)\n", inliers.size(), ucpts.size(), quality);

	if (quality < inlierPercentage)
		return false;

	float error = reprojError(inliers);
	bool badreproj = (error > maxReprojError) || (error < 0.01f);
	
	if (!badreproj) printf("reproj %fpx\n", error);

	return !badreproj;
}

//draw debug information
void KylesNFT::debug() {
	cv::Point pd = cv::Point(3, 3);
	cv::Point pd2 = cv::Point(3, -3);

	cv::Mat tmp;
	tmpFrame.copyTo(tmp);

	// project whole feature cloud as [ ]'s
	for (int i = 0; i < reprojpts.size(); ++i) {
		cv::Point cvp = cv::Point(reprojpts[i].x, reprojpts[i].y);
		cv::rectangle(tmp, cvp - pd, cvp + pd, cv::Scalar(255, 0, 255), 2);
	}

	// draw recognized points as X's
	if (ucpts.size() > 0) {
		for (int i = 0; i < ucpts.size(); ++i) {
			cv::Point cvp = cv::Point(ucpts[i].x, ucpts[i].y);
			cv::line(tmp, cvp - pd, cvp + pd, cv::Scalar(0, 0, 255), 2);
			cv::line(tmp, cvp - pd2, cvp + pd2, cv::Scalar(0, 0, 255), 2);
		}
	}

	imshow("results", tmp);
}

//do all the things
void KylesNFT::update(cv::Mat &curFrame) {

	preProcess(curFrame);

	if (!detectAndCompute(tmpFrame, cfts, cdesc)) {
		//printf("not enough features\n");
		return;
	}

	if (!match(fdesc, cdesc, m)) {
		//printf("not enough matches\n");
		return;
	}

	cpts.clear();
	ucpts.clear();
	fpts.clear();

	//extract points, sort by matches
	for (int i = 0; i < m.size(); ++i) {
		cpts.push_back(cfts[m[i].trainIdx].pt);
		fpts.push_back(ftCloud[m[i].queryIdx]);
	}

	//undistort cam points into 'normalized' coordinates
	//undistortPoints(cpts, ucpts, cam->rgb.camMat, cam->rgb.camDist);
	//undistort cam points into camMat coordinates
	undistortPoints(cpts, ucpts, cam->rgb.camMat, cam->rgb.camDist, noArray(), cam->rgb.camMat);

	if (!getMatrix()) {
		//printf("not enough inliers\n");
		return;
	}

	debug();

}
