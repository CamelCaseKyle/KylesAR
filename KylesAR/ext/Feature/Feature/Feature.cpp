#include "feature.h"

////////////////////////////////////////////////////////////////// Feature Point //////////////////////////////////////////////////////////////////

//NaturalFeatureKalman::NaturalFeatureKalman() : ExtendedKalman(1, 3) {}
////state transition function (usually identity or sensor calibration)
//cv::Mat NaturalFeatureKalman::f(cv::Mat _x) {}
////jacobian of state transition (usually identity matrix, unless calibration)
//cv::Mat NaturalFeatureKalman::getF(cv::Mat _x) {}
////observation function (array of m elements predicting observations)
//cv::Mat NaturalFeatureKalman::h(cv::Mat _x) {}
////jacobian of observation (m x n matrix)
//cv::Mat NaturalFeatureKalman::getH(cv::Mat _x) {}


//useable init
FeatureTrack::FeatureTrack(std::string nom, R200* _cam, const float* obj, const int nPts) : pose(nom) {
	//rvec = cv::Mat(3, 1, CV_64FC1);
	//tvec = cv::Mat(3, 1, CV_64FC1);

	name = nom;
	cam = _cam;

	setModel(obj, nPts);

	//initialize threads
	//extractFeatures = std::thread(this->getAllPoints, nullptr);
	//estimatePose = std::thread(this->poseEstimation, nullptr);
}


//set the cad model to be tracked
void FeatureTrack::setModel(const float* obj, const int nPoints) {
	for (int i = 0; i < nPoints * 3; i += 3) objPoints.push_back(cv::Point3f(obj[i], obj[i + 1], obj[i + 2]));
}

//set initial guess for model
void FeatureTrack::setPose(cv::Mat& _rvec, cv::Mat& _tvec) {
	_rvec.copyTo(rvec);
	_tvec.copyTo(tvec);
}


//check against the last projected points bounding box
inline bool FeatureTrack::segmentPoint(const cv::Point& p) {
	//int offsetx = cam->frame.cols >> 2,
	//	offsety = cam->frame.rows >> 2;
	//ignore everything that is not towards the center of the screen
	//if (p.x < offsetx || p.x > cam->frame.cols - offsetx) return false;
	//if (p.y < offsety || p.y > cam->frame.rows - offsety) return false;

	return true;
}

//matches points from segment against projected objPoints and places them (in order) into the *matches vars
void FeatureTrack::matchPoints() {
	Points projected;

	if (rvec.empty() || tvec.empty() || objPoints.empty()) return;

	//get new pose from kalman filter


	//assume this preserves order, project object points
	cv::projectPoints(objPoints, rvec, tvec, cam->camMat, cam->camDist, projected);

	//find segment points closest to projected points
	Points possiblePoints;
	objMatches.clear();
	segMatches.clear();
	float halfMinDist = featPtMinDist / 2.f;
	int proInd = 0;

	//loop through projected points and search possible points
	for (auto pro = projected.begin(); pro != projected.end(); pro++, proInd++) {
#ifdef DBG_FEATURE
		cv::line(debugDisp, *pro, *pro, cv::Scalar(0, 0, 255), 4);
#endif
		for (auto seg = segment.begin(); seg != segment.end(); seg++) {
			float dx = pro->x - seg->x, dy = pro->y - seg->y, dist = sqrt(dx*dx + dy*dy);
			if (dist < halfMinDist) {
				objMatches.push_back(objPoints[proInd]);
				segMatches.push_back(*seg);
				break;
			}
		}
	}

#ifdef DBG_FEATURE
	cv::imshow("NFT", debugDisp);
#endif
	//dont wait here, wait later
}

//runs on its own thread (updates pose output, inliers)
bool FeatureTrack::poseEstimation() {
	//while (1) {

	//if there arent enough features to solve PnP
	if (segMatches.size() != objMatches.size() || segMatches.size() < 4 || rvec.empty() || tvec.empty()) {
		//needInitialPose = true;
		return false;
	}

	//increase reliability of inlier points
	cv::solvePnP(objMatches, segMatches, cam->camMat, cam->camDist, rvec, tvec, !needInitialPose); // , 100, 4.0f, 0.98);

	//convert pose rvec, tvec to mat4 specification
	getMatrix();

	//needInitialPose = false;

	Points projected;
	//assume this preserves order, project object points
	cv::projectPoints(objPoints, rvec, tvec, cam->camMat, cam->camDist, projected);
	//loop through projected points and search possible points
	for (auto pro = projected.begin(); pro != projected.end(); pro++) cv::line(debugDisp, *pro, *pro, cv::Scalar(0, 255, 0), 4);

#ifdef DBG_FEATURE
	cv::imshow("NFT", debugDisp);
	cv::waitKey(1);
#endif

	return true;

	//}
}


//convert to standard
void FeatureTrack::getMatrix() {
	rvec.copyTo(pose.rvec);
	tvec.copyTo(pose.tvec);

	//get the matricies for OpenGL
	cv::Mat rotation(3, 3, CV_64FC1), viewMatrix(4, 4, CV_64FC1);
	//convert rvec to rotation matrix
	cv::Rodrigues(rvec, rotation);

	//fix and build the matrix
	for (unsigned int row = 0; row <3; row++) {
		for (unsigned int col = 0; col <3; col++) {
			viewMatrix.at<double>(row, col) = rotation.at<double>(row, col);
		}
		viewMatrix.at<double>(row, 3) = tvec.at<double>(row, 0);
	}
	viewMatrix.at<double>(3, 3) = 1.0f;

	//invert the y and z axis
	cv::Mat cvToGl = cv::Mat::zeros(4, 4, CV_64FC1);
	cvToGl.at<double>(0, 0) = 1.0f; cvToGl.at<double>(3, 3) = 1.0f;
	cvToGl.at<double>(1, 1) = -1.0f; cvToGl.at<double>(2, 2) = -1.0f; //inv lol

	viewMatrix = cvToGl * viewMatrix;

	//transpose copy
	for (unsigned int row = 0; row < 3; row++) {
		pose.view[row].x = viewMatrix.at<double>(row, 0);
		pose.view[row].y = viewMatrix.at<double>(row, 1);
		pose.view[row].z = viewMatrix.at<double>(row, 2);
		pose.view[row].w = viewMatrix.at<double>(row, 3);
	}

	//inverse rotation
	cv::Mat R = rotation.t();
	//compuate inverse translation
	cv::Mat t = -R * tvec;
	//start with identity
	pose.view_cam = glm::mat4(1.0);
	//transpose and copy
	for (unsigned int row = 0; row < 3; row++) {
		pose.view_cam[row].x = R.at<double>(row, 0);
		pose.view_cam[row].y = R.at<double>(row, 1);
		pose.view_cam[row].z = R.at<double>(row, 2);
		pose.view_cam[row].w = t.at<double>(row);
	}
}

//runs on its own thread every so often (updates segment)
void FeatureTrack::getAllPointsOnce(cv::Mat& frame) {
	//clear last results
	feats.clear();
	//clear last segmented points or reproject for search?
	segment.clear();
	//cam frame to GSC
	cv::Mat frameGSC, frameBlur;

#ifdef DBG_FEATURE
	frame.copyTo(debugDisp);
#endif

	//greyscale (use SV stream?)
	cv::cvtColor(frame, frameGSC, CV_BGR2GRAY);
	//difference of box filters
	cv::blur(frameGSC, frameBlur, cv::Size(4, 4));
	cv::absdiff(frameGSC, frameBlur, frameGSC);
	//find based on input
	cv::goodFeaturesToTrack(frameGSC, feats, featPtMax, featPtQual, featPtMinDist);
	//dont include points that dont meet criteria
	for (int i = 0; i < feats.size(); i++) {
		if (!segmentPoint(feats[i])) continue;
		//segment the point becasue it meets criteria
		segment.push_back(cv::Point(feats[i].x, feats[i].y));

#ifdef DBG_FEATURE
		cv::line(debugDisp, segment.back(), segment.back(), cv::Scalar(0, 0, 0), 3);
#endif

	}
}

//just accessor, doesnt modify/update anything
Points* FeatureTrack::getSegment() {
	return &segment;
}


//estimate lighting using feature points' attribs or something
std::vector<LightObject*> FeatureTrack::getLights() {
	std::vector<LightObject *> output;
	return output;
}

//should only have 1 output
std::vector<PositionObject*> FeatureTrack::getPositions() {
	std::vector<PositionObject*> out;

	//get all pts
	getAllPointsOnce(cam->frame);

	//match pionts for pose estimation
	matchPoints();

	//if we got a pose, add it to output
	if (poseEstimation()) {
		pose.ID = poseBaseID;
		out.push_back(&pose);
	}

	//return ...stuff
	return out;
}
