#include "symbol.h"

//something else
SymbolInput::SymbolInput(std::string nom) : SceneObject(nom), TouchInput() {}

//scene graphical init
SymbolInput::SymbolInput(std::string nom, BINDINGS _binding, BIND_BEHAVIOR _behavior, int pID) : SceneObject(nom), TouchInput() {
	cont.model = glm::mat4( 
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f, 
		0.f, 0.f, 1.f, 0.f, 
		-20.f, 0.f, 0.f, 1.f 
	);
	cont.view = glm::mat4(1.0);
	cont.view_cam = glm::mat4(1.0);
	cont.normalMatrix = glm::mat4(1.0);

	info.binding = _binding;
	info.behavior = _behavior;
	info.desiredPoseID = pID;
}
//override button setup except setup the input buffer
void SymbolInput::setup_occlusion(int oc_width, int oc_height, float cam_fov, Shader* _ocShade) {
	TouchInput::setup_occlusion(oc_width, oc_height, cam_fov, _ocShade);

	//setup input buffer
	//ib = cv::Mat::zeros(oc_height, oc_width, CV_8UC3);
	//rangeHi = 16;

	//load templates
}
//set up textures
void SymbolInput::setup_textures(cv::Mat &active, cv::Mat &inactive, Shader* _shade) {
	active.copyTo(tex_active);
	
	inactive.copyTo(tex_inactive);
	cont.glTex[t_active] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	uploadTexture(tex_inactive, cont.glTex[t_active]);

	cont.shade = _shade;

	_textures = true;
}
//set up gpu geometry
void SymbolInput::setup_geometry(const float *v, const float *n, const float *u, const unsigned int *i, int nPts, int nInds) {
	TouchInput::setup_geometry(v, n, u, i, nPts, nInds);

	cont.verts = v;
	cont.norms = n;
	cont.uvs = u;
	cont.inds = i;
	cont.n_verts = nPts * 3;
	cont.n_uvs = nPts * 2;
	cont.n_inds = nInds;

	cont.glVAO[a_quad] = createVertexArray(false);

	//vert buffer
	cont.glVBO[v_quad_verts] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &cont.verts[0], cont.n_verts * sizeof(float), cont.glVBO[v_quad_verts], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//norm buffer
	cont.glVBO[v_quad_norms] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &cont.norms[0], cont.n_verts * sizeof(float), cont.glVBO[v_quad_norms], false);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//uv buffer
	cont.glVBO[v_quad_uvs] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &cont.uvs[0], cont.n_uvs * sizeof(float), cont.glVBO[v_quad_uvs], false);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	cont.glVBO[v_quad_ind] = createBuffer();
	uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, &cont.inds[0], cont.n_inds * sizeof(unsigned int), cont.glVBO[v_quad_ind], false);

	glBindVertexArray(0);

	_geometry = true;
}

//load templates from file
void SymbolInput::loadTemplates() {
	//load list of 2D points from file

	//symbol:R
	//length:100
	//points:20
	//shape {}
	
}
//compare seg with templates using usdf
int SymbolInput::matchTemplates() {
	//might have more points than seg
	std::vector<cv::Point> newSeg;
	//remainder of segment division, step length
	float rem = 0.f, step = 4.f;

	//convert seg into normalized length line segments
	for (int i = 1; i < seg.size(); i++) {
		//get length between them
		cv::Point d = seg[i] - seg[i - 1];
		//current dist + remainder from last time
		float segdist = sqrt(d.x*d.x + d.y*d.y) + rem;

		//if greater than step, slice up
		if (segdist > step) {
			int slices = int(segdist / step);
			float cdist = 0.;
			for (int j = 0; j < slices; j++) {
				//linear interpolation
				float ratio = step / (segdist - cdist);
				newSeg.push_back(seg[i - 1] * (1.f - ratio) + seg[i] * ratio);
				//increment dist
				cdist += step;
			}
			//there will probably be some remainder
			rem = segdist - cdist;

		}
		else {
			//dont slice the segment, record progress and continue
			rem = segdist;

		}

	}

	//different strategy: see how stable the distance trace is, get tangent difference (lastDist - dist)
	//get less points if distance changes too much (path of least resistance
	//check newSeg total length against template

	//*first is average distnace, second is furthest away
	std::vector<std::pair<float, float>> scores;
	//running counts
	float total = 0.f, lowest = 255.f, nsize = float(newSeg.size());

	/*test all of newSeg against each distance feild
	for (auto t : templates) {
		total = 0.f;
		lowest = 255.f;
		//newSeg is more likely to be cache friendly
		for (auto p : newSeg) {
			float s = float(t.data[int(p.y)*t.step + int(p.x)*t.channels()]);
			total += s;
			if (s < lowest) lowest = s;
		}
		scores.push_back(std::pair<float, float>(total / nsize, lowest));
	} //*/

	float bestScore = -1.f;
	int bestInd = 0, scoreInd = 0;
	for (int i = 0; i < scores.size(); i++) {
		if (scores[i].first > bestScore) {
			bestScore = scores[i].first;
			bestInd = scoreInd;
		}
		scoreInd++;
	}

	std::string results[] = { "invalid", "<", ">", "R" };
	printf("symbol %s\n", results[bestInd].c_str());

	return bestInd; //template match failed
}

void SymbolInput::sceneCallback(Scene* scene, Context* renderer) {
	SceneObject::sceneCallback(scene, renderer);
	if (hasOcclusion()) occont.target = renderer;
}
//bind to parent callback
void SymbolInput::bindCallback(PositionObject* parent) {
	cont.model = glm::mat4(
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		-parent->size - 10.0f, 0.f, 0.f, 1.f
	); 
}
//remove from scene callback
//void SymbolInput::removeCallback() {}


//similar to button update, handle drawing
void SymbolInput::update() {

	//get new touches
	TouchInput::update();

	if ((time & 3) != 0) return;

	//handle input
	if (!isOccluded() && (seg.size() > 0)) {
		inpClrTime++;

		if (inpClrTime > delay) {
			//match against templates
			matchTemplates();

			//reset input buffer
			ib = cv::Mat::zeros(ocuv.rows, ocuv.cols, CV_8UC3);
			//delete segment
			seg.clear();
			//reset time
			inpClrTime = 0;
			//upload fresh texture (switched because of hover() code)
			uploadTexture(tex_inactive, cont.glTex[t_active]);
		}
	}
	
	if (isIntersected()) {
		//reset time
		inpClrTime = 0;
		//get new touches, now you may be thinking "singleTouch" but we want to match it to seg, not lastTouch
		std::vector<cv::Point> res = multiTouch();

		//if no touch data or invalid touch data
		if (res.size() == 0 || res[0].x == -1) return;
		
		if (seg.size() == 0) {
			//else start segment with point
			seg.push_back(res[0]);
			//draw a green dot
			cv::line(ib, seg.front(), seg.front(), cv::Scalar(0, 192, 0), 5);
		} else {
			//draw segment
			cv::line(ib, seg.back(), res[0], cv::Scalar(192, 98, 0), 3);
			//add to input seg
			seg.push_back(res[0]);

		}

		//start with highlighted texture
		cv::Mat temp(tex_active.rows, tex_active.cols, ib.type());
		//scale ib to tex_active size
		cv::resize(ib, temp, temp.size());
		//add on
		cv::add(tex_active, temp, temp);

		//update on GPU (tex id has been swapped by hover() code)
		uploadTexture(temp, cont.glTex[t_active]);
	}
}
//similar to button render, just draw a quad
void SymbolInput::render() {
	
	if (!_textures || !_geometry) {
		printf("symbol input no textures or geometry\n");
		return;
	}

	if (hasOcclusion()) {
		//update matrix
		occont.model = cont.model;
		occont.view = cont.view;
		occont.view_cam = cont.view_cam;
		//render updated ocuv to gpu
		TouchInput::render();
	}

	//aliasing saves keystrokes and cpu cycles
	Shader& shade = *cont.shade;
	Context& txt = *cont.target;

	//then render to fbo vertex:
	glBindFramebuffer(GL_FRAMEBUFFER, txt.fbo[txt.f_vertex]);

	glUseProgram(shade.getID());
	glBindVertexArray(cont.glVAO[a_quad]);

#ifdef BUILD_GLASSES
	glUniform2f(shade.uniform("frameSize"), 2.f / txt.viewp[2], 1.f / txt.viewp[3]);
#else
	glUniform2f(shade.uniform("frameSize"), 1.f / txt.viewp[2], 1.f / txt.viewp[3]);
#endif

	//uniform matrix
	glm::mat4 mv = cont.view * cont.model,
		mvp = txt.projection * mv,
		normal = glm::inverse(glm::transpose(mv));

	glUniformMatrix4fv(shade.uniform("MVP"), 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(shade.uniform("MV"), 1, GL_FALSE, &mv[0][0]);
	glUniformMatrix4fv(shade.uniform("M"), 1, GL_FALSE, &cont.model[0][0]);
	glUniformMatrix4fv(shade.uniform("normalMatrix"), 1, GL_FALSE, &normal[0][0]);

	//bind texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cont.glTex[t_active]);
	glUniform1i(shade.uniform("texture"), 0);

	//depth if we have it
	if (shade.uniform("depthFrame")) {
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, txt.resID[txt.r_depth]);
		glUniform1i(shade.uniform("depthFrame"), 1);
	}
	
	// Draw the square!
	glDrawElements(GL_TRIANGLES, cont.n_inds, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glDisable(cont.glTex[t_active]);
	if (shade.uniform("depthFrame")) glDisable(txt.resID[txt.r_depth]);
}
