/* SLAM is a huge problem

EKF or other filter:
	cv::KalmanFilter
Bundle adjustment:
	cv::detail::BundleAdjusterReproj
Better feature points:
	cv::AKAZE 
Feature point localization
Loop Closure

Views:
	Trajectory
	Feature Points
	Feature Matching

*/

#include "stdafx.h"
#include "SLAM.h"

//rotation may be transposed
void getMatrix(PositionObject& pose) {
	for (int i = 0; i < 3; i++) pose.view[i] = glm::vec4(pose.rvec.at<double>(0, i), pose.rvec.at<double>(1, i), pose.rvec.at<double>(2, i), 0.);
	pose.view[3] = glm::vec4(pose.tvec.at<double>(0), pose.tvec.at<double>(1), pose.tvec.at<double>(2), 1.f);

	//invert the y and z axis
	glm::mat4 cv2gl = glm::mat4(1.0f);
	cv2gl[1].y = -1.0f; cv2gl[2].z = -1.0f;
	//apply transform (UNDO: transpose(view))
	pose.view = cv2gl * pose.view;
	
#ifdef LOG_POSE
	printf("SLAM Pose:\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		pose.view[0][0], pose.view[0][1], pose.view[0][2], pose.view[0][3],
		pose.view[1][0], pose.view[1][1], pose.view[1][2], pose.view[1][3],
		pose.view[2][0], pose.view[2][1], pose.view[2][2], pose.view[2][3],
		pose.view[3][0], pose.view[3][1], pose.view[3][2], pose.view[3][3]
	);
#endif

	//inverse translation
	cv::Mat t = -pose.rvec.t() * pose.tvec;
	//build matrix transposed
	for (int i = 0; i < 3; i++) pose.view_cam[i] = glm::vec4(pose.rvec.at<double>(0, i), pose.rvec.at<double>(1, i), pose.rvec.at<double>(2, i), 0.);
	pose.view_cam[3] = glm::vec4(t.at<double>(0), t.at<double>(1), t.at<double>(2), 1.0f);
}

SLAM::SLAM() : pose("SLAM") {
	pose.rvec = Mat::eye(3, 3, CV_64FC1);
	pose.tvec = Mat::zeros(3, 1, CV_64FC1);
	pose.size = 1.f;

	fast = cv::FastFeatureDetector::create();
	//sift = ... ;
	flann = cv::DescriptorMatcher::create("BruteForce");
}

SLAM::~SLAM() {}

void SLAM::setCam(i_Camera* _cam) {
	//new input
	cam = _cam;
	cvtColor(cam->rgb.frame, cf, CV_BGR2GRAY);
	resize(cf, cf, Size(cf.cols >> 1, cf.rows >> 1));
	//SLAM needs additional info
	cv::Matx33d camMat = cam->camMat;
	camMat(0, 0) *= .5;
	camMat(0, 1) *= .5;
	camMat(0, 2) *= .5;
	camMat(1, 0) *= .5;
	camMat(1, 1) *= .5;
	camMat(1, 2) *= .5;
	calibrationMatrixValues(cam->camMat, Size(cf.cols, cf.rows), apWidth, apHeight, fovx, fovy, flen, pp, aspect);
	//visualization
	traj = Mat::zeros(cf.size(), cf.type());
	//success
	hasCam = true;
}
//use GoodFeaturesToTrack
void SLAM::detect(Mat& inp, Points& outp) {
	goodFeaturesToTrack(inp, outp, 1000, .333, 5.);
	if (outp.size()) cornerSubPix(inp, outp, Size(3, 3), Size(-1, -1), TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 0.1));
}
//use FAST
void SLAM::detect(Mat& inp, Feats& finp, Mat& outp) {
	bool useFinp = (finp.size() > 5);
	fast->detectAndCompute(inp, noArray(), finp, outp, useFinp);
}
//use LkOpticalFlow
void SLAM::track(Mat& prevf, Mat& curf, Points& prevp, Points& curp, Floats& qual, UChars& status) {
	Floats err; //not used
	calcOpticalFlowPyrLK(prevf, curf, prevp, curp, status, err);
	//getting rid of points for which the LK tracking failed (or those who have gone outside the frame)
	int indexCorrection = 0;
	for (int i = 0; i < status.size(); i++) {
		int ind = i - indexCorrection;
		Point2f pt = curp.at(ind);
		//measure corner fitness?
		if (status.at(i) == -1 || pt.x < 0 || pt.y < 0 || pt.x >= curf.cols || pt.y >= curf.rows) {
			prevp.erase(prevp.begin() + ind);
			curp.erase(curp.begin() + ind);
			indexCorrection++;
		}
	}
}
//use KNN Matcher
void SLAM::match(Mat& in1, Mat& in2, Feats& f1, Feats& f2, Matches& outp) {
	//knnMatch?
	outp.clear();
	Matches tmp;
	flann->match(in1, in2, tmp);
	//percentage of image size?
	const double tDistSquared = 1024.;
	//filter out matches whos points are not close
	for (size_t i = 0; i < tmp.size(); ++i) {
		Point2f from = f1[tmp[i].queryIdx].pt;
		Point2f to = f2[tmp[i].trainIdx].pt;
		//calculate local distance for each possible match
		double dist = (from.x - to.x) * (from.x - to.x) + (from.y - to.y) * (from.y - to.y);
		//save as best match if local distance is in specified area and on same height
		if (dist < tDistSquared) outp.push_back(tmp[i]);
	}
}

vector<PositionObject*> SLAM::getPositions() {
	vector<PositionObject*> outp;
	if (!hasCam) return outp;

	//last state
	cf.copyTo(lf);
	//lpts = cpts;
	lfts = cfts;
	ldesc = cdesc;

	Point lpt = Point((cf.cols >> 1) + int(pose.view[3][0] * trajScale), (cf.rows >> 1) + int(pose.view[3][1] * trajScale));

	//new input
	resize(cam->rgb.frame, cf, Size(0, 0), .5, .5);
	cvtColor(cf, cf, CV_BGR2GRAY);
	cf *= 1.5;
	
	//populate points
	//detect(cf, cpts);
	detect(cf, cfts, cdesc);

	if (cpts.size() >= 5) {
		//track points over time
		//track(lf, cf, lpts, cpts, qpts, ptstat);

		Matches m;
		match(cdesc, ldesc, cfts, lfts, m);

		//choose best > 5 with qual?
		Points flp, fcp;
		//iterate through qpts

		//have to check again, track might modify
		//if (cpts.size() > 4) {
		if (m.size() > 4) {
		
			//undistort cam points
			//undistortPoints(lpts, flp, cam->camMat, cam->camDist);
			//undistortPoints(cpts, fcp, cam->camMat, cam->camDist);
			undistortPoints(lfts, flp, cam->camMat, cam->camDist);
			undistortPoints(cfts, fcp, cam->camMat, cam->camDist);

			//pose step 1
			Mat E = findEssentialMat(flp, fcp, flen, pp, RANSAC, .9, 2., femstat);

			if (E.rows != 3 || E.cols != 3) {
				printf("bad Essential mat\n");
				return outp;
			} else {
				Points nlp, ncp;
				for (int i = 0; i < femstat.size(); i++) {
					if (femstat[i]) {
						nlp.push_back(flp[i]);
						ncp.push_back(fcp[i]);
					}
				}

				if (nlp.size() < 5) {
					printf("not enough inliers\n");
					return outp;
				}

				//pose output format
				Mat rm = Mat(3, 3, CV_64FC1), tv = Mat(3, 1, CV_64FC1);
				//pose step 2
				int inliers = recoverPose(E, nlp, ncp, rm, tv, flen, pp);
				
				if (inliers > 4) {
					//get scale from pose.size
					tv *= pose.size;
					//add relitive transformation onto trajectory (post-mult)
					pose.tvec = pose.tvec + (pose.rvec * tv);
					pose.rvec = pose.rvec * rm;

					//handle pose format
					getMatrix(pose);
				} else {
					printf("recoverPose not enough inliers\n");
					return outp;
				}
			}
		}
	}

	//draw points and motion
	for (int i = 0; i < cpts.size(); i++) cv::line(cf, lpts[i], cpts[i], Scalar(255), 3);
	imshow("Features", cf);
	
	//draw trajectory
	Point pt = Point((cf.cols >> 1) + int(pose.view[3][0] * trajScale), (cf.rows >> 1) + int(pose.view[3][1] * trajScale));
	cv::line(traj, lpt, pt, Scalar(128 + int(pose.view[3][2]) * 10.), 3);
	imshow("Trajectory", traj);

	outp.push_back(&pose);
	return outp;
}

void SLAM::originCallback(PositionObject* newOrigin) {

}
void SLAM::pauseTracking() {

}
void SLAM::resumeTracking() {

}
