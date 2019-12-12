#include "parallaxSFM.h"

ParallaxSFM::ParallaxSFM(Context *rend) {
	GLsizei w = GLsizei(rend->viewp[2]), h = GLsizei(rend->viewp[3]);
	string paraCWD = getCWD();

	//renderer
	paraRend = rend;

	//create an FBO to render normal map to
	createFullScreenPassNoDepth(paraCont, 0, w, h, GL_NEAREST, GL_NEAREST, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	createFullScreenPassNoDepth(paraCont, 1, w, h, GL_NEAREST, GL_NEAREST, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
	createFullScreenPassNoDepth(paraCont, 2, w, h, GL_NEAREST, GL_NEAREST, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);

	//perspective warp and encode shader
	encodeShade = Shader(paraCWD + "experiments\\glsl\\encode.vert", paraCWD + "experiments\\glsl\\encode.frag");
	encodeShade.addUniform("curFrame");
	encodeShade.addUniform("frameSize");
	encodeShade.addUniform("M");

	//disparity shader
	disparityShade = Shader(paraCWD + "experiments\\glsl\\disparity.vert", paraCWD + "experiments\\glsl\\disparity.frag");
	disparityShade.addUniform("curFrame");
	disparityShade.addUniform("wrpFrame");
	disparityShade.addUniform("frameSize");

	//post process disparity shader
	//postShade = Shader(paraCWD + "experiments\\glsl\\post.vert", paraCWD + "experiments\\glsl\\post.frag");

	//textures
	paraCont.glTex[0] = createTexture(GL_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP);
	paraCont.glTex[1] = createTexture(GL_NEAREST, GL_LINEAR_MIPMAP_LINEAR, GL_CLAMP);

}

void ParallaxSFM::compute(cv::Mat curFrame, glm::mat4 curPose) {
	//get 2 consecutive frames and marker poses

	if (glm::distance(curPose[3], lastPose[3]) < .01)
		return;

	if (firstFrame) {
		curFrame.copyTo(lastFrame);
		//upload keyframe (eventually just select it from the circular buffer)
		uploadTexture(lastFrame, paraCont.glTex[1], true);
		lastPose = curPose;
		firstFrame = false;
		return;
	}

	//upload current frame
	uploadTexture(curFrame, paraCont.glTex[0], true);
	//encode, no warp
	paraCont.shade = &encodeShade;
	paraCont.model = glm::mat4(1.);
	paraRend->RenderUnindexedContent(paraCont, 0, 0, { paraCont.glTex[0] }, { "curFrame" });

	//warp keyframe into current frame system
	glm::vec2 midpt(float(curFrame.cols) * .5f, float(curFrame.rows) * .5f);
	glm::mat4
		VP = paraRend->projection,
		cMVP = VP * curPose,
		lMVP = VP * lastPose;
	const vector<glm::vec4> quadPoints = {
		glm::vec4(7.5f, 0.f,  7.5f, 1.f),
		glm::vec4(-7.5f, 0.f,  7.5f, 1.f),
		glm::vec4(-7.5f, 0.f, -7.5f, 1.f),
		glm::vec4(7.5f, 0.f, -7.5f, 1.f)
	};
	vector<cv::Point2f> cvCpts(4), cvLpts(4);
	for (int i = 0; i < 4; ++i) {
		glm::vec4 ct = cMVP * quadPoints[i],
			lt = lMVP * quadPoints[i];
		ct /= ct.w;
		lt /= lt.w;
		cvCpts[i] = cv::Point2f(-ct.x, ct.y);
		cvLpts[i] = cv::Point2f(-lt.x, lt.y);
	}

	//calc perspective
	cv::Mat M1, M, tmp = cv::Mat(curFrame.size(), curFrame.type());
	M1 = cv::getPerspectiveTransform(cvLpts, cvCpts);
	cv::invert(M1, M);

	//encode and warp
	paraCont.shade = &encodeShade;
	paraCont.model[0] = glm::vec4(M.at<double>(0, 0), M.at<double>(1, 0), M.at<double>(2, 0), 0.);
	paraCont.model[1] = glm::vec4(M.at<double>(0, 1), M.at<double>(1, 1), M.at<double>(2, 1), 0.);
	paraCont.model[2] = glm::vec4(M.at<double>(0, 2), M.at<double>(1, 2), M.at<double>(2, 2), 0.);
	paraCont.model[3] = glm::vec4(0., 0., 0., 1.);
	paraRend->RenderUnindexedContent(paraCont, 1, 0, { paraCont.glTex[1] }, { "wrpFrame" });

	//do the disparity
	paraCont.shade = &disparityShade;
	paraRend->RenderUnindexedContent(paraCont, 2, 0, { paraCont.glRenTex[0], paraCont.glRenTex[1] }, { "curFrame", "wrpFrame" });

	// draw
	//cv::Mat hcat;
	//cv::hconcat(vector<cv::Mat>({ curFrame, lastFrame }), hcat);
	//cv::line(hcat, cvCpts[0], cvCpts[0], cv::Scalar(255, 0, 0), 5);
	//cv::line(hcat, cvCpts[1], cvCpts[1], cv::Scalar(0, 255, 0), 5);
	//cv::line(hcat, cvCpts[2], cvCpts[2], cv::Scalar(0, 0, 255), 5);
	//cv::line(hcat, cvCpts[3], cvCpts[3], cv::Scalar(255, 0, 255), 5);

	cv::Mat
		//rt0 = cv::Mat::zeros(curFrame.size(), curFrame.type()),
		rt1 = cv::Mat::zeros(curFrame.size(), curFrame.type()),
		rt2 = cv::Mat::zeros(curFrame.size(), curFrame.type());
	//downloadTexture(rt0, paraCont.glRenTex[0]);
	downloadTexture(rt1, paraCont.glRenTex[1]);
	downloadTexture(rt2, paraCont.glRenTex[2]);

	cv::imshow("rt1", rt1);
	cv::imshow("rt2", rt2);
}
