#include "planeDetection.h"

//plane detection setup
PlaneDetection::PlaneDetection(Context &rend) {
	//quarter size
	GLsizei w = GLsizei(rend.viewp[2]) >> 1, h = GLsizei(rend.viewp[3]) >> 1;
	//create an FBO to render normal map to
	createFullScreenPassNoDepth(pd_cont, 0, w, h, GL_NEAREST, GL_NEAREST, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
	//create a mat to hold result on CPU
	nrml = cv::Mat(h, w, CV_8UC3);
	//renderer
	pdrend = &rend;
	//shader
	pdcwd = getCWD();
	normShade = Shader(pdcwd + "experiments\\glsl\\pd.vert", pdcwd + "experiments\\glsl\\pd.frag");
	//shader uniforms
	normShade.addUniform("vertFrame");
	normShade.addUniform("frameSize");
}

//plane detection
void PlaneDetection::update(cv::Point _p) {
	cv::Point p(_p.x, _p.y);
	//render scene mesh over normal buffer so we dont recompute a bunch
	downloadTexture(nrml, pd_cont.glRenTex[0]);
	if (nrml.rows * nrml.cols <= 0) return;
	//	also it will seed verts that are very close to existing ones for matching
	//posterize normal map
	cv::Mat posnrm = nrml, outp = cv::Mat::zeros(nrml.rows, nrml.cols, CV_8UC3);
	cv::bitwise_and(nrml, cv::Scalar(240, 240, 240), posnrm);

	//get posterized normal under p
	uchar *nrmdat = posnrm.data;
	unsigned int smp = p.y*posnrm.step + p.x * 3;
	uchar nb = nrmdat[smp], ng = nrmdat[smp + 1], nr = nrmdat[smp + 2];
	nb = (nb < 16) ? 16 : (nb > 240) ? 240 : nb;
	ng = (ng < 16) ? 16 : (ng > 240) ? 240 : ng;
	nr = (nr < 16) ? 16 : (nr > 240) ? 240 : nr;
	//threshold similar normals
	cv::Mat thresh;
	cv::inRange(posnrm, cv::Scalar(nb - 32, ng - 32, nr - 32), cv::Scalar(nb + 64, ng + 64, nr + 64), thresh);
	//approx poly with higher pixel error
	vector<vector<cv::Point2i>> blobs;
	vector<cv::Point2i> blob;
	vector<cv::Vec4i> unused;
	//get shape of area sharing normal
	cv::findContours(thresh, blobs, unused, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
	if (blobs.size() == 0) return;
	//triangulate mesh
	for (int k = 0; k < blobs.size(); k++) {
		cv::approxPolyDP(blobs[k], blob, 5., true);
		//if this poly contains p
		if (cv::pointPolygonTest(blob, p, false) > 0.) {
			//tag each 'ear' if not concave

			//push back inds, pop tagged points

			//just draw for now

		}
	}
	//project into camera space

	//transform with inverse of pose to get to world space

	//add/merge verts, inds, norms (uvs, texture?) to scene mesh

	//just show for now
	cv::imshow("pd", thresh);
}

//normal map render
void PlaneDetection::render() {
	//compute normal map 1/4 size renderbuffer

	//render to our fbo
	glBindFramebuffer(GL_FRAMEBUFFER, pd_cont.glFBO[0]);

	//clear the FBO as this is the first write of a frame
	glClear(GL_COLOR_BUFFER_BIT);

	//use the transform shader
	Shader &shade = normShade;
	glUseProgram(shade.getID());

	//use the projection vertex array
	glBindVertexArray(pd_cont.glVAO[0]);

	//transform matrix
	glUniform2f(shade.uniform("frameSize"), 2.f / pdrend->viewp[2], 2.f / pdrend->viewp[3]);

	//sensor data
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pdrend->resID[pdrend->r_vert]);
	glUniform1i(shade.uniform("vertFrame"), 0);

	// Draw the square!
	glDrawArrays(GL_TRIANGLES, 0, pd_cont.n_verts);

	glBindVertexArray(0);
	glDisable(pdrend->resID[pdrend->r_vert]);

}
