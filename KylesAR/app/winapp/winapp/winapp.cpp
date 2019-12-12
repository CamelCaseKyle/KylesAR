#include "stdafx.h"
#include "winapp.h"

//something else
WinApp::WinApp(std::string _exe) : i_SceneObject(), i_OcclusionFeedback() { name = _exe; }

//scene graphical init
WinApp::WinApp(std::wstring _path, std::wstring _exe, BINDINGS _binding, BIND_BEHAVIOR _behavior, int pID) : i_SceneObject(), i_OcclusionFeedback() {
	name = toStr(_exe); 
	cont.model = glm::mat4(1.f);
	cont.view = glm::mat4(1.f);
	cont.view_cam = glm::mat4(1.f);
	cont.normalMatrix = glm::mat4(1.f);
	info.binding = _binding;
	info.behavior = _behavior;
	info.desiredPoseID = pID;	
	win = WindowsIntegration(_path, _exe);
	_scene = true;
}

//set occlusion buffer properties, set w/h to 0 for size based on win RECT
void WinApp::setup_occlusion(int oc_width, int oc_height) {
	//check state
	if (!hwnd || oc_width * oc_height <= 0) {
		printf("Winapp: %s setup_occlusion() fail (w:%i h:%i hasHWND:%s)\n", name, oc_width, oc_height, hwnd ? "true" : "false");
		return;
	}
	i_OcclusionFeedback::setup_occlusion(oc_width, oc_height, nullptr);
}
//set up gpu geometry
void WinApp::setup_geometry() {
	i_OcclusionFeedback::setup_geometry(win_verts, win_norms, win_uvs, win_inds, 4, 6);
	//content!
	cont.verts = win_verts;
	cont.norms = win_norms;
	cont.uvs = win_uvs;
	cont.inds = win_inds;
	cont.n_verts = 12;
	cont.n_uvs = 8;
	cont.n_inds = 6;

	cont.glVAO[a_quad] = createVertexArray(false);

	//vert buffer
	cont.glVBO[v_quad_verts] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cont.verts, cont.n_verts * sizeof(float), cont.glVBO[v_quad_verts], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//norm buffer
	cont.glVBO[v_quad_norms] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cont.norms, cont.n_verts * sizeof(float), cont.glVBO[v_quad_norms], false);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//uv buffer
	cont.glVBO[v_quad_uvs] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cont.uvs, cont.n_uvs * sizeof(float), cont.glVBO[v_quad_uvs], false);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	cont.glVBO[v_quad_ind] = createBuffer();
	uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, cont.inds, cont.n_inds * sizeof(unsigned int), cont.glVBO[v_quad_ind], false);

	glBindVertexArray(0);

	cont.glVAO[a_back] = createVertexArray(false);

	//back vert buffer
	cont.glVBO[v_back_quad_verts] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, win_back_verts, cont.n_verts * sizeof(float), cont.glVBO[v_back_quad_verts], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//back norm buffer
	cont.glVBO[v_back_quad_norms] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, win_back_norms, cont.n_verts * sizeof(float), cont.glVBO[v_back_quad_norms], false);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//back uv buffer
	cont.glVBO[v_back_quad_uvs] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cont.uvs, cont.n_uvs * sizeof(float), cont.glVBO[v_back_quad_uvs], false);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//back indices
	cont.glVBO[v_back_quad_ind] = createBuffer();
	uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, cont.inds, cont.n_inds * sizeof(unsigned int), cont.glVBO[v_back_quad_ind], false);

	glBindVertexArray(0);

	_geometry = true;
}
//set up textures
void WinApp::setup_shader(bool hasDepth) {

	cont.glTex[t_back] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	cv::Mat back = cv::Mat(1, 1, CV_8UC3, cv::Scalar(128, 64, 0));
	uploadTexture(back, cont.glTex[t_back]);
	cont.glTex[t_win] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	_textures = true;

	// load shaders
	std::string path = getCWD();

	//shared uniforms between shaders
	std::vector<std::string> winUniforms = { "texture", "frameSize", "MVP", "MV", "M", "normalMatrix", "light_power", "light_color", "light_cameraspace" };
	std::string vertPath = "", fragPath = "";
	//choose the version with depth support
	if (hasDepth) {
		vertPath = path + std::string("\\glsl\\winapp.depth.vert");
		fragPath = path + std::string("\\glsl\\winapp.depth.frag");
		//depth uniforms
		winUniforms.push_back("depthFrame");
		winUniforms.push_back("depthSize");
	} else {
		//no extra uniforms
		vertPath = path + std::string("\\glsl\\winapp.vert");
		fragPath = path + std::string("\\glsl\\winapp.frag");
	}
	winShade = Shader(vertPath, fragPath);
	if (winShade.getID() == NULL) return;

	//add all the uniforms
	winShade.addUniforms(winUniforms);
	//use as main shader
	cont.shade = &winShade;
}

//scene init callback
void WinApp::sceneCallback(Scene* scene, Context* renderer) {
	i_OcclusionFeedback::sceneCallback(scene, renderer);
	info.parent = scene;
	cont.target = renderer;
	int gwidth = win.picture.cols >> 2, gheight = win.picture.rows >> 2;
	setup_shader(scene->getCamera()->depth.exists);
	setup_occlusion(gwidth, gheight);
	//change screen geometry size,
	float win_cpy[12], back_cpy[12],
		scl = float(gwidth) * .1f,
		ratio = float(gheight) / float(gwidth);
	for (int i = 0; i < 12; i++) win_cpy[i] = win_verts[i] * scl;
	//aspect fix
	win_cpy[7] *= ratio;
	win_cpy[10] *= ratio;
	for (int i = 0; i < 12; i++) back_cpy[i] = win_back_verts[i] * scl;
	back_cpy[4] *= ratio;
	back_cpy[7] *= ratio;
	//upload geometry
	uploadBuffer(GL_ARRAY_BUFFER, &win_cpy[0], cont.n_verts * sizeof(float), cont.glVBO[v_quad_verts]);
	uploadBuffer(GL_ARRAY_BUFFER, &back_cpy[0], cont.n_verts * sizeof(float), cont.glVBO[v_back_quad_verts]);
	//upload occlusion geometry
	uploadBuffer(GL_ARRAY_BUFFER, &win_cpy[0], occont.n_verts * sizeof(float), occont.glVBO[v_ocmesh_verts]);
}
//bind to parent callback (doesnt apply to world objects)
void WinApp::bindCallback(PositionObject* parent) {}
//remove from scene callback
void WinApp::removeCallback() {}

//launch and hook windowsintegration
bool WinApp::launch() {
	win.start();
	//wait for app
	Sleep(4000);
	//auto hook
	win.hook();
	//wait for app
	Sleep(1000);
	//check for success
	hwnd = false;
	if (win.isInitialized()) hwnd = true;
	return hwnd;
}

//similar to button update, handle drawing
void WinApp::update() {
	//get new touches
	if (hasOcclusion()) i_OcclusionFeedback::update();
	else time ++;
	//throttle
	if ((time & 3) != 0) return;
	//if we dont have window dont do anything
	if (!hwnd) {
		//check again just in case
		if (win.isInitialized()) hwnd = true;
		else {
			if (info.parent == nullptr) return;
			printf("Winapp: %s no HWND, removing self\n", name);
			info.parent->removeContent((i_SceneObject *)this);
			return;
		}
	}

	win.capture();
	
	if (isIntersected()) {
		//get new touches, now you may be thinking "singleTouch" but we want to match it to seg, not lastTouch
		std::vector<cv::Point> res = multiTouch();
		//if no touch data or invalid touch data
		if (res.size() == 0) return;
		if (res[0].x < 1) return;
		//scale from oc buffer size to window size
		float scaleX = float(win.picture.cols) / float(ocuv.cols),
			  scaleY = float(win.picture.rows) / float(ocuv.rows);
		cv::Point loc = cv::Point(int(float(res[0].x) * scaleX), int(float(res[0].y) * scaleY));
		//draw it
		cv::circle(win.picture, loc, 33, cv::Scalar(255, 128, 0), 2);
		cv::circle(win.picture, loc, 35, cv::Scalar(128, 64, 0), 2);
		//send touches as clicks (invert y)
		win.sendClick(cv::Point(loc.x, win.picture.rows - loc.y));
	}

	uploadTexture(win.picture, cont.glTex[t_win]);

}
//similar to button render, just draw a quad
void WinApp::render() {

	if (!_geometry || !_textures) {
		printf("Winapp %s%s\n", _textures ? "" : "no textures ", _geometry ? "" : "no geometry");
		return;
	}

	if (cont.shade == nullptr || cont.target == nullptr) {
		printf("Winapp%s%snot set up\n", cont.shade == nullptr ? " shade " : " ", cont.target == nullptr ? " target " : " ");
		return;
	}

	if (hasOcclusion()) {
		//update matrix
		occont.model = cont.model;
		occont.view = cont.view;
		//render updated ocuv to gpu
		i_OcclusionFeedback::render();
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
	glBindTexture(GL_TEXTURE_2D, cont.glTex[t_win]);
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
	glDisable(cont.glTex[t_win]);
	if (shade.uniform("depthFrame")) glDisable(txt.resID[txt.r_depth]);

	///////////////////////////////////////////////////////////////////////////////////////

	glUseProgram(shade.getID());
	glBindVertexArray(cont.glVAO[a_back]);

#ifdef BUILD_GLASSES
	glUniform2f(shade.uniform("frameSize"), 2.f / txt.viewp[2], 1.f / txt.viewp[3]);
#else
	glUniform2f(shade.uniform("frameSize"), 1.f / txt.viewp[2], 1.f / txt.viewp[3]);
#endif

	glUniformMatrix4fv(shade.uniform("MVP"), 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(shade.uniform("MV"), 1, GL_FALSE, &mv[0][0]);
	glUniformMatrix4fv(shade.uniform("M"), 1, GL_FALSE, &cont.model[0][0]);
	glUniformMatrix4fv(shade.uniform("normalMatrix"), 1, GL_FALSE, &normal[0][0]);

	//bind texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cont.glTex[t_back]);
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
	glDisable(cont.glTex[t_back]);
	if (shade.uniform("depthFrame")) glDisable(txt.resID[txt.r_depth]);
}
