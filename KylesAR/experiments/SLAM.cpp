#include "SLAM.h"

SLAM::SLAM(i_Camera *inpCam) {
	cam = inpCam;

	pose.rvec = Mat::eye(3, 3, CV_64FC1);
	pose.tvec = Mat::zeros(3, 1, CV_64FC1);
	pose.size = 1.f;

	//visualization
	processFrame(cam->rgb.frame, cf);
	traj = Mat::zeros(cf.cols, cf.cols, CV_8UC3);
	mask = Mat(cf.rows, cf.cols, CV_8UC1, Scalar(255));
	lpt = Point((traj.cols >> 4) + int(pose.view[3][0] * trajScale), (traj.cols >> 1) - int(pose.view[3][2] * trajScale));

	//mask for finding feature points
	int mx = cf.cols >> 1, my = cf.rows >> 1,
		mw = mx >> 3, mh = my >> 3;
	cv::rectangle(mask, Rect(mx - mw, my - mh, mw << 1, mh << 1), Scalar(0), -1);

}

void SLAM::getMatrix(PositionObject &pose) {
	// put into GLM mat4
	for (int i = 0; i < 3; i++) pose.view[i] = glm::vec4(pose.rvec.at<double>(0, i), pose.rvec.at<double>(1, i), pose.rvec.at<double>(2, i), 0.);
	pose.view[3] = glm::vec4(-pose.tvec.at<double>(1), pose.tvec.at<double>(2), -pose.tvec.at<double>(0), 1.f);
	// filter it
	fltr.update(pose.view);

	////invert the x and z axis
	//glm::mat4 cv2gl = glm::mat4(
	//	0.f, 1.f, 0.f, 0.f,
	//	0.f, 0.f, 1.f, 0.f,
	//	1.f, 0.f, 0.f, 0.f,
	//	0.f, 0.f, 0.f, 1.f
	//);
	////apply transform (UNDO: transpose(view))
	//pose.view = pose.view * cv2gl;

#ifdef LOG_POSE
	printf("SLAM Pose:\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n%f, %f, %f, %f\n",
		pose.view[0][0], pose.view[0][1], pose.view[0][2], pose.view[0][3],
		pose.view[1][0], pose.view[1][1], pose.view[1][2], pose.view[1][3],
		pose.view[2][0], pose.view[2][1], pose.view[2][2], pose.view[2][3],
		pose.view[3][0], pose.view[3][1], pose.view[3][2], pose.view[3][3]
	);
#endif

	////inverse translation
	//cv::Mat t = -pose.rvec.t() * pose.tvec;
	////build matrix transposed
	//for (int i = 0; i < 3; i++) pose.view_cam[i] = glm::vec4(pose.rvec.at<double>(0, i), pose.rvec.at<double>(1, i), pose.rvec.at<double>(2, i), 0.);
	//pose.view_cam[3] = glm::vec4(t.at<double>(0), t.at<double>(1), t.at<double>(2), 1.0f);
}

void SLAM::detect(Mat &inp, Feats &inout1, Mat &out1) {
	//maybe if we used optical flow
	bool useinout = false; //(inout1.size() > 5);
	akaze->detectAndCompute(inp, mask, inout1, out1, useinout);
	printf("detect %i\n", out1.rows);
}

void SLAM::match(Mat &in1, Mat &in2, Feats &f1, Feats &f2, Matches &outp) {
	//knnMatch?
	outp.clear();
	Matches tmp;
	bfm->match(in1, in2, tmp);
	//percentage of image size? based on current motion?
	const float tMax = 100.f;
	//filter out matches whos points are not close
	for (size_t i = 0; i < tmp.size(); ++i) {
		Point2f p1 = f1[tmp[i].queryIdx].pt;
		Point2f p2 = f2[tmp[i].trainIdx].pt;
		float dx = (p1.x - p2.x), dy = (p1.y - p2.y),
			dist = dx * dx + dy * dy;
		if (dist < tMax)
			outp.push_back(tmp[i]);
	}
	printf("match %i %i (%f%%)\n", int(outp.size()), int(tmp.size()), float(outp.size()) / float(tmp.size()) * 100.f);
}

void SLAM::processFrame(Mat &inp, Mat &outp) {
	printf("size: %i %i, type: %i\n", inp.cols, inp.rows, inp.type());
	cv::resize(inp, outp, Size(0, 0), .5, .5);
	cv::cvtColor(outp, outp, CV_BGR2GRAY);
}

void SLAM::show() {
	Mat tmp;
	cf.copyTo(tmp);
	//draw points and motion
	int sz = (lpts.size() < cpts.size()) ? lpts.size() : cpts.size();
	for (int i = 0; i < sz; i++) cv::line(tmp, cpts[i], lpts[i], cv::Scalar(255 * femstat[i]), 2);
	if (tmp.cols * tmp.rows) cv::imshow("Features", tmp);

	//draw trajectory
	traj = traj * 0.95;
	//get new pose
	glm::mat4 npos = fltr.predict();
	Point pt = Point((traj.cols >> 1) + int(npos[3][0] * trajScale), (traj.cols >> 1) - int(npos[3][2] * trajScale));

	float ts = trajScale * 8.;
	cv::line(traj, pt, Point(pt.x + int(npos[0][0] * ts), pt.y - int(npos[0][1] * ts)), Scalar(0, 0, 255), 1);
	cv::line(traj, pt, Point(pt.x + int(npos[1][0] * ts), pt.y - int(npos[1][1] * ts)), Scalar(0, 255, 0), 1);
	cv::line(traj, pt, Point(pt.x + int(npos[2][0] * ts), pt.y - int(npos[2][1] * ts)), Scalar(255, 0, 0), 1);

	if (traj.cols > 0 && traj.rows > 0) cv::imshow("Trajectory", traj);
}

float SLAM::reprojError(PositionObject *curPose, PositionObject *lastPose, vector<cv::Point3f> out3d) {
	Points CimgPts, LimgPts;
	// project points into 'normalized' coordinates because of which undistortPoints we chose
	cv::projectPoints(out3d, curPose->rvec, curPose->tvec, cv::Matx33f::eye(), cv::noArray(), CimgPts);
	cv::projectPoints(out3d, lastPose->rvec, lastPose->tvec, cv::Matx33f::eye(), cv::noArray(), LimgPts);

	// out3d, cpts, and lpts are correlated by index
	float error = 0.f;
	for (int i = 0; i < out3d.size(); i++) {
		Point2f d = LimgPts[i] - lpts[i];
		error += d.dot(d);
		d = CimgPts[i] - cpts[i];
		error += d.dot(d);
	}
	error /= float(out3d.size() * 2);
	error = glm::sqrt(error);

	printf("reproj error %f\n", error);

	return error;
}

void SLAM::update() {
	show();

	lpt = Point((traj.cols >> 4) + int(pose.view[3][0] * trajScale), (traj.cols >> 1) - int(pose.view[3][2] * trajScale));

	//clear
	cpts.clear();
	lpts.clear();
	//last state
	lfts = cfts;
	ldesc = cdesc;
	lf = cf;
	processFrame(cam->rgb.frame, cf);

	//populate points
	detect(cf, cfts, cdesc);

	//create matches
	Matches m;
	match(ldesc, cdesc, lfts, cfts, m);

	if (m.size() < 5) {
		printf("not enough matches\n");
		return;
	}

	//extract points, sort by matches
	for (int i = 0; i < m.size(); ++i) {
		cpts.push_back(cfts[m[i].trainIdx].pt);
		lpts.push_back(lfts[m[i].queryIdx].pt);
	}

	//undistort cam points (if using r200)
	//undistortPoints(cpts, cpts, cam->camMat, cam->camDist);
	//undistortPoints(lpts, lpts, cam->camMat, cam->camDist);

	//find fundamental mat?
	cv::Mat F = findFundamentalMat(lpts, cpts, CV_FM_RANSAC, 1., .9999, femstat);

	//correct matches?
	printf("F rows: %i, F cols: %i, lpts size: %i, cpts size: %i\n", F.rows, F.cols, int(lpts.size()), int(cpts.size()));
	if (F.cols != 3 || F.rows != 3) {
		printf("could not calc fundamental\n");
		return;
	}

	correctMatches(F, lpts, cpts, lpts, cpts);

	//find essential mat
	Mat E = findEssentialMat(lpts, cpts, flen, pp, CV_RANSAC, .9999, 1., femstat);

	//filter outliers using femstat

	printf("essential %i\n", int(lpts.size()));

	if (E.rows != 3 || E.cols != 3) {
		printf("bad Essential mat\n");
		return;
	}

	//pose output format
	Mat rm = Mat(3, 3, CV_64FC1), tv = Mat(3, 1, CV_64FC1);
	//pose step 2
	int inliers = recoverPose(E, lpts, cpts, rm, tv, flen, pp);

	if (inliers > 4) {
		printf("inliers %f%%\n", float(inliers) / float(cpts.size()) * 100.f);

		//get scale from pose.size
		tv *= pose.size;
		//add relitive transformation onto trajectory (post-mult)
		pose.tvec = pose.tvec + (pose.rvec * tv);
		pose.rvec = pose.rvec * rm;

		//handle pose format
		getMatrix(pose);

		/* do some reconstruction?

		cv::Mat rotMat(3, 3, CV_64F);
		// current pose
		cv::Mat augMatC(3, 4, CV_64F);
		// convert rotation vector into rotation matrix
		cv::Rodrigues(curPose->rvec, rotMat);
		// append translation vector to rotation matrix
		cv::hconcat(rotMat, curPose->tvec, augMatC);

		// last pose
		cv::Mat augMatL(3, 4, CV_64F);
		cv::Rodrigues(lastRvec, rotMat);
		cv::hconcat(rotMat, lastTvec, augMatL);

		out4d = cv::Mat(4, cpts.size(), CV_32F);
		// triangulate into 4D homogenous coords
		cv::triangulatePoints(augMatL, augMatC, lpts, cpts, out4d);

		//cv::Mat p3D(p4D.rows - 1, p4D.cols, p4D.type());
		// omfg the simplest function doesnt even work WOW
		//convertPointsFromHomogeneous(p4D, p3D);

		// my implementation
		vector<cv::Point3f> p3D;
		for (int i = 0; i < cpts.size(); ++i) {
			cv::Point3f pt = cv::Point3f(p4D.at<float>(0, i), p4D.at<float>(1, i), p4D.at<float>(2, i));
			p3D.push_back(pt / p4D.at<float>(3, i));
		}

		// now we have a pointcloud between the last two frames of video
		// this should be joined/stored in a spacial acceleration structure when you decide to take a 'keyframe'

		*/

	} else {
		printf("recoverPose only %i inliers\n", inliers);
	}
}
