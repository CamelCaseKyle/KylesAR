#include "i_ocfb.h"

//get small things like fingers intersecting with plane
void i_OcclusionFeedback::processMultiTouch() {
	if (!_occlusion || ocuv.rows == 0 || ocuv.cols == 0) return;

	cv::Mat inp;
	std::vector<std::vector<cv::Point2i>> tmp;
	std::vector<cv::Point2i> contour;
	std::vector<cv::Vec4i> hierarchy;

	cv::inRange(ocuv, cv::Scalar(rangeLo), cv::Scalar(255), inp);
	occluders = cv::countNonZero(inp) > int((float)inp.rows * (float)inp.cols * 0.01f);

	cv::inRange(ocuv, cv::Scalar(rangeLo), cv::Scalar(rangeHi), inp);
	cv::blur(inp, inp, cv::Size(10, 10));
	cv::inRange(inp, cv::Scalar(98), cv::Scalar(255), inp);
#ifdef DEBUG_OCFB
	cv::imshow("ocfb inp", inp);
#endif
	cv::findContours(inp, tmp, hierarchy, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

	//if no contours
	if (tmp.size() == 0) {
		//raw test fail
		intersected = false;
		return;
	}

	std::vector<std::pair<cv::Point2i, float>> tmpTouches;
	cv::Point2f circleCenter;
	float circleRadius, area;

	//shape filter by complexity, size
	for (int h = 0; h < tmp.size(); h++) {
		cv::approxPolyDP(tmp[h], contour, 2, true);
		//complexity
		if (contour.size() < 3 || contour.size() > 32) continue;
		//size
		area = float(cv::contourArea(contour));
		if (area < 25 || area > 325) continue;
		//get a more stable centroid
		cv::minEnclosingCircle(contour, circleCenter, circleRadius);
		//found
		tmpTouches.push_back(TouchEvent(circleCenter, area));
	}

	//we have 3 frames worth of touch data, should mebeh find angle \/
	if (tmpTouches.size() > 0) {
		lastTouches = curTouches;
		curTouches = tmpTouches;
		//raw test pass
		intersected = true;
	}
}

//let people overload this so i can get their info
void i_OcclusionFeedback::setup_geometry(const float *v, const float *n, const float *u, const unsigned int *i, int nPts, int nInds) {
	occont.verts = v;
	occont.uvs = u;
	occont.inds = i;
	occont.n_verts = nPts * 3;
	occont.n_uvs = nPts * 2;
	occont.n_inds = nInds;

	occont.glVAO[a_ocmesh] = createVertexArray(false);

	//rendering to the occlusion uv map instead of the screen
	occont.glVBO[v_ocmesh_uv] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &occont.uvs[0], occont.n_uvs * sizeof(float), occont.glVBO[v_ocmesh_uv], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	//transform verts manually to sample transformed depth map
	occont.glVBO[v_ocmesh_verts] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &occont.verts[0], occont.n_verts * sizeof(float), occont.glVBO[v_ocmesh_verts], false);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

	//objects index buffer
	occont.glVBO[v_ocmesh_ind] = createBuffer();
	uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, &occont.inds[0], occont.n_inds * sizeof(unsigned int), occont.glVBO[v_ocmesh_ind], false);

	glBindVertexArray(0);

	_geometry = true;
}
//create occlusion buffer, feedback and render passes
void i_OcclusionFeedback::setup_occlusion(int oc_width, int oc_height, Shader *_ocShade) {
	//occlusion feedback shader
	occont.model = glm::mat4(1.f);
	occont.view = glm::mat4(1.f);
	occont.view_cam = glm::mat4(1.f);
	occont.normalMatrix = glm::mat4(1.f);
	if (_ocShade != nullptr) occont.shade = _ocShade;
	else occont.shade = &OcclusionFeedbackShader;
	//set up oc buffer
	ocuv = cv::Mat::zeros(oc_height, oc_width, CV_8UC1);

	//oc buffer to gl type lookup
	std::vector<int> &ocinfo = textureHelper[ocuv.type()];
	occont.glRenTex[r_ocout] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP, false);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, ocinfo[0], ocuv.cols, ocuv.rows, 0, ocinfo[1], ocinfo[2], 0);
	occont.glFBO[f_ocbuff] = createFrameBuffer(occont.glRenTex[r_ocout], false);

	//best check ever
	GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ||
		status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ||
		status == GL_FRAMEBUFFER_UNSUPPORTED) {
		printf("Failed to generate framebuffer for TouchInput: status=%i\n", status);
		return;
	}

	//unbind
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	_occlusion = true;
}

//get informed about everything
void i_OcclusionFeedback::sceneCallback(Scene *scene, Context *renderer) {
	occont.target = renderer;
}

//order output points by time, match over time
std::vector<cv::Point> i_OcclusionFeedback::multiTouch() {
	std::vector<cv::Point2i> outp;
	for (int i = 0; i < curTouches.size(); i++) outp.push_back(curTouches[i].first);
	return outp;
}
//narrow multitouch to single smoothed touch event
cv::Point2i i_OcclusionFeedback::singleTouch() {
	cv::Point ret(-1, -1);
	freshData = false;

	//handle input
	if (!occluders && freshData) {
		clearTime++;

		if (clearTime > clearDelay) {
			//stale data
			freshData = false;
			//reset time
			clearTime = 0;
		}
	}

	if (intersected) {
		//reset time
		clearTime = 0;

		if (!freshData) {
			float gtSize = 0.;
			int foundInd = -1;
			//choose largest point to start?
			for (int j = 0; j < curTouches.size(); j++) {
				//if smaller
				if (gtSize > curTouches[j].second) continue;
				//new size
				gtSize = curTouches[j].second;
				foundInd = j;
			}
			//if not found
			if (foundInd < 0) return ret;

			//else start segment with largest point
			lastSingle = curSingle;
			curSingle = curTouches[foundInd].first;
			freshData = true;
			return curSingle;
		}
		//get closest points with min threshold
		int leastDist = 9999, foundInd = -1;
		for (int j = 0; j < curTouches.size(); j++) {
			//get least dist squared
			int x = curTouches[j].first.x - lastSingle.x;
			int y = curTouches[j].first.y - lastSingle.y;
			int dist = x * x + y * y;
			//if greater dist
			if (leastDist < dist || dist > 1400 || dist < 4) continue;
			//new dist
			leastDist = dist;
			foundInd = j;
		}

		if (foundInd < 0) return ret;

		lastSingle = curSingle;
		curSingle = cv::Point(
			(int)smoothx.filter((double)curTouches[foundInd].first.x),
			(int)smoothy.filter((double)curTouches[foundInd].first.y)
		);
		freshData = true;
		return curSingle;
	}
	return ret;
}

//update cpu oc buffer
void i_OcclusionFeedback::update() {
	if ((++time & 3) != 0) return;

	render();

	//get from GPU
	downloadTexture(ocuv, occont.glRenTex[r_ocout]);

#ifdef DEBUG_OCFB
	cv::imshow("ocuv", ocuv);
#endif

	//get fingertips hopefully
	processMultiTouch();
}
//render occlusion buffer
void i_OcclusionFeedback::render() {

	if (!_occlusion || !_geometry || occont.shade == nullptr || occont.target == nullptr) {
#ifdef DEBUG_OCFB
		printf("occlusion render not set up\n");
#endif
		return;
	}

	glFrontFace(GL_CW);
	glViewport(0, 0, ocuv.cols, ocuv.rows);

	//first render to occlusion buffer:
	glBindFramebuffer(GL_FRAMEBUFFER, occont.glFBO[f_ocbuff]);
	glClear(GL_COLOR_BUFFER_BIT);

	//aliasing saves keystrokes and cpu cycles
	Shader &shade = *occont.shade;
	Context &txt = *occont.target;

	glUseProgram(shade.getID());
	glBindVertexArray(occont.glVAO[a_ocmesh]);

	glm::mat4 mvp = occont.target->projection * occont.view * occont.model;

	//uniforms
	glUniformMatrix4fv(shade.uniform("MVP"), 1, GL_FALSE, &mvp[0][0]);

	//bind depth for testing
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, txt.resID[txt.r_vert]);
	glUniform1i(shade.uniform("depthFrame"), 0);

	//issue the draw call
	glDrawElements(GL_TRIANGLES, occont.n_inds, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glDisable(txt.resID[txt.r_depth]);

	glViewport(0, 0, GLsizei(txt.viewp[2]), GLsizei(txt.viewp[3]));
	glFrontFace(GL_CCW);
}
