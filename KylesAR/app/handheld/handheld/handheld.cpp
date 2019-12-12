#include "stdafx.h"
#include "handheld.h"

HandheldInput::HandheldInput(int pID)
//	blue("BlueButton", BIND_PARENT, PARENT_ID, pID),
//	green("GreenButton", BIND_PARENT, PARENT_ID, pID) 
{
	cont.model = glm::mat4(1.f);
	cont.view = glm::mat4(1.f);
	cont.view_cam = glm::mat4(1.f);
	cont.normalMatrix = glm::mat4(1.f);

	info.binding = BIND_PARENT;
	info.behavior = PARENT_ID;
	info.desiredPoseID = pID;
	
	screen = cv::Mat(128, 128, CV_8UC3, cv::Scalar(0, 0, 0));
	laser = cv::Mat(1, 200, CV_8UC1, cv::Scalar(255));

	std::string cwd = getCWD(),
		bgdir = cwd + "/images/bg_a.png",
		titledir = cwd + "/images/title.png",
		cursordir = cwd + "/images/cursor.png",
		ffdir = cwd + "/images/firefox.png",
		symboldir = cwd + "/images/symbolinput.png",
		huddir = cwd + "/images/hud.png",
		personaldir = cwd + "/images/personal.png",
		parentdir = cwd + "/images/parent.png",
		worlddir = cwd + "/images/world.png";

	bgs.resize(nBackgrounds);
	bgs[b_idle] = cv::cvarrToMat(cvLoadImage(bgdir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	bgs[b_title] = cv::cvarrToMat(cvLoadImage(titledir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	bgs[b_cursor] = cv::cvarrToMat(cvLoadImage(cursordir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	
	pgmICOs.resize(nProgramIcons);
	pgmICOs[p_firefox] = cv::cvarrToMat(cvLoadImage(ffdir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	pgmICOs[p_symbol] = cv::cvarrToMat(cvLoadImage(symboldir.c_str(), CV_LOAD_IMAGE_UNCHANGED));

	bindICOs.resize(nBindIcons + 1); //1 for purgatory lolol
	bindICOs[BIND_HUD] = cv::cvarrToMat(cvLoadImage(huddir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	bindICOs[BIND_PERSONAL] = cv::cvarrToMat(cvLoadImage(personaldir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	bindICOs[BIND_PARENT] = cv::cvarrToMat(cvLoadImage(parentdir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	bindICOs[BIND_WORLD] = cv::cvarrToMat(cvLoadImage(worlddir.c_str(), CV_LOAD_IMAGE_UNCHANGED));
	bindICOs[BIND_PURGATORY] = cv::Mat(bindICOs[BIND_HUD].size(), bindICOs[BIND_HUD].type(), cv::Scalar(0, 0, 0));

	_textures = true;
}
//partial setup
bool HandheldInput::setup_shaders() {
	//render screen
	state = s_idle;
	renderState();

	//upload screen
	cont.glTex[t_active] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	uploadTexture(screen, cont.glTex[t_active]);

	//upload laser
	cont.glTex[t_laser] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	uploadTexture(laser, cont.glTex[t_laser]);

	//damn glitchy winderps interface
	//cv::Mat active, inactive;

	//active = cv::cvarrToMat(cvLoadImage("images/button_b_a.png"));
	//inactive = cv::cvarrToMat(cvLoadImage("images/button_b_i.png"));
	//blue.setup_textures(active, inactive, true);
	
	//active = cv::cvarrToMat(cvLoadImage("images/button_g_a.png"));
	//inactive = cv::cvarrToMat(cvLoadImage("images/button_g_i.png"));
	//green.setup_textures(active, inactive, true);

	//enable the green button
	//green.swapTextures();


	// load shaders
	std::string path = getCWD();

	//shared uniforms between shaders
	std::vector<std::string> uniforms = { "texture", "frameSize", "MVP", "MV", "M", "normalMatrix", "light_power", "light_color", "light_cameraspace" };
	std::string vertPath = path + std::string("/glsl/handheld.vert"),
				fragPath = path + std::string("/glsl/handheld.frag"),
				laserVert = path + std::string("/glsl/cursor.vert"),
				laserFrag = path + std::string("/glsl/cursor.frag");

	screenShade = Shader(vertPath, fragPath);
	screenShade.addUniforms(uniforms);

	laserShade = Shader(laserVert, laserFrag);
	laserShade.addUniforms(uniforms);

	if (screenShade.getID() == NULL || laserShade.getID() == NULL) return false;

	cont.shade = &screenShade;

	_shade = true;

	return true;
}
//sets up geometry and GPU presence
void HandheldInput::setup_geometry() {
	i_OcclusionFeedback::setup_geometry(cursor_verts, cursor_norms, cursor_uvs, handheld_inds, 4, 6);
	cont.verts = handheld_verts;
	cont.norms = handheld_norms;
	cont.uvs = handheld_uvs;
	cont.inds = handheld_inds;
	cont.n_verts = 12;
	cont.n_uvs = 8;
	cont.n_inds = 6;
	
	cont.glVAO[a_screen] = createVertexArray(false);

	//vert buffer
	cont.glVBO[v_screen_verts] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cont.verts, cont.n_verts * sizeof(float), cont.glVBO[v_screen_verts], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//norm buffer
	cont.glVBO[v_screen_norms] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cont.norms, cont.n_verts * sizeof(float), cont.glVBO[v_screen_norms], false);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//uv buffer
	cont.glVBO[v_screen_uvs] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cont.uvs, cont.n_uvs * sizeof(float), cont.glVBO[v_screen_uvs], false);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	cont.glVBO[v_screen_ind] = createBuffer();
	uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, cont.inds, cont.n_inds * sizeof(unsigned int), cont.glVBO[v_screen_ind], false);

	glBindVertexArray(0);

	cont.glVAO[a_laser] = createVertexArray(false);

	//vert buffer
	cont.glVBO[v_laser_verts] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cursor_verts, cont.n_verts * sizeof(float), cont.glVBO[v_laser_verts], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//norm buffer
	cont.glVBO[v_laser_norms] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cursor_norms, cont.n_verts * sizeof(float), cont.glVBO[v_laser_norms], false);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//uv buffer
	cont.glVBO[v_laser_uvs] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, cursor_uvs, cont.n_uvs * sizeof(float), cont.glVBO[v_laser_uvs], false);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	cont.glVBO[v_laser_ind] = createBuffer();
	uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, cont.inds, cont.n_inds * sizeof(unsigned int), cont.glVBO[v_laser_ind], false);

	glBindVertexArray(0);

	//blue.setup_geometry(&button_verts[0], &button_norms[0], &button_uvs[0], &button_inds[0], 4, 6);
	//green.setup_geometry(&button_verts[0], &button_norms[0], &button_uvs[0], &button_inds[0], 4, 6);
	//if (blue.hasGeometry() && green.hasGeometry())
		_geometry = true;
}
//set up occlusion feedback
void HandheldInput::setup_occlusion(int oc_width, int oc_height) {
	if (oc_width * oc_height <= 0) {
		printf("Handheld: %s setup_occlusion() fail (w:%i h:%i)\n", name, oc_width, oc_height);
		return;
	}
	i_OcclusionFeedback::setup_occlusion(oc_width, oc_height, nullptr);
	//the screen does not support occlusion feedback
	//blue.setup_occlusion(oc_width, oc_height, cam_fov, nullptr);
	//button is 6x3cm so that makes 10 pixels per cm
	//green.setup_occlusion(oc_width, oc_height, cam_fov, nullptr);
}

//add to scene callback
void HandheldInput::sceneCallback(Scene* scene, Context* renderer) {
	i_OcclusionFeedback::sceneCallback(scene, renderer);
	info.parent = scene;
	cont.target = renderer;
	setup_occlusion(laser.cols, laser.rows);

	//now we can search for the button
	Module<i_SceneObject*>* button = scene->getModuleByName<i_SceneObject*>("Button");
	if (button == nullptr) return;
	//if the module is not in memory
	if (!button->shouldLoad()) {
		button->shouldLoad(true);
		button->init();
	}
	//create a few instances
	if (button->isOpen()) {
		blue = button->newInst();
		green = button->newInst();
	}

	//should call blue and greens sceneCallback's too? wont it get called already?
	//blue.info.parent = scene;
	//blue.cont.target = renderer;
	//blue.occont.target = renderer;
	//green.info.parent = scene;
	//green.cont.target = renderer;
	//green.occont.target = renderer;

	_scene = true;
}
//bind to parent callback
void HandheldInput::bindCallback(PositionObject* parent) {
	//pass binding onto children
	info.pose = parent;
	//blue.info.pose = parent;
	//green.info.pose = parent;
	//we got a new marker size probably
	//blue.cont.model = glm::mat4(
	//	1.f, 0.f, 0.f, 0.f,
	//	0.f, 1.f, 0.f, 0.f,
	//	0.f, 0.f, 1.f, 0.f,
	//	-3.5f, 1.5f, -parent->size - 3.f, 1.f
	//);
	//green.cont.model = glm::mat4(
	//	1.f, 0.f, 0.f, 0.f,
	//	0.f, 1.f, 0.f, 0.f,
	//	0.f, 0.f, 1.f, 0.f,
	//	3.5f, 1.5f, -parent->size - 3.f, 1.f
	//);
	//change screen geometry size
	float scr_cpy[24];
	for (int i = 0; i < 12; i++) scr_cpy[i] = handheld_verts[i] * parent->size;
	//change laser parameters
	for (int i = 12; i < 24; i++) scr_cpy[i] = handheld_verts[i];
	scr_cpy[14] *= parent->size + 2.f;
	scr_cpy[20] *= parent->size + 2.f;
	uploadBuffer(GL_ARRAY_BUFFER, &scr_cpy[0], cont.n_verts * sizeof(float), cont.glVBO[v_screen_verts]);
}

//marches depth image
void HandheldInput::traceDepth() {
	if (!hasOcclusion()) return;
	//uploadTexture(ocuv, cont.glTex[t_laser]);

	//march from left to right until intersected
	int count = -1, nCols = ocuv.cols, i = 0, j = 0, l = getRangeLow(), h = getRangeHigh();
	uchar* p = ocuv.ptr<uchar>(i);
	for (j = 10; j < nCols; ++j) {
		uchar pj = p[j];
		if (int(pj) > l) {
			if (count > 1) {
				i = j;
				break;
			} else count++;
		} else count = 0;
	}
	if (i == 0) i = ocuv.cols;
	//draw a white line to there, black line from there til end
	laser = cv::Mat::zeros(laser.size(), laser.type());
	cv::line(laser, cv::Point(0, 0), cv::Point(std::max(0, int(float(i) * .9f) - 4), 0), cv::Scalar(255));
	
	//cv::Mat sclzr;
	//cv::resize(laser, sclzr, cv::Size(), 1., 100.);
	//cv::imshow("sclzr", sclzr);

	//re upload
	uploadTexture(laser, cont.glTex[t_laser]);
}

//state functions
void HandheldInput::process_idle() {
	/*

	//check if world objects or parents are near
	std::vector<SceneObject*> objs = info.parent->getObjectsByBinding(BIND_WORLD);
	std::vector<PositionObject*> poses = info.parent->getPoses();

	//the cursor's location is (cont.world * cursor.cont.model)[3]
	glm::vec3 cursorLoc = glm::vec3((cont.view * cont.model)[3]);
	//find least dist
	float leastDist = 999999.9f;
	bool leastIsPose = false;
	int leastInd = -1;

	//search world objects
	for (int i = 0; i < objs.size(); i++) {
		//a world object's location is (obj.world * obj.model)[3]
		glm::vec3 objectLoc = glm::vec3((objs[i]->cont.view * objs[i]->cont.model)[3]),
			diff = cursorLoc - objectLoc;
		float dist = glm::dot(diff, diff);
		if (dist < leastDist) {
			leastDist = dist;
			leastInd = i;
			leastIsPose = false;
		}
	}

	//search parent poses
	for (int i = 0; i < poses.size(); i++) {
		//a marker's location is just (pose.world_cam)[3]
		glm::vec3 markerLoc = glm::vec3(poses[i]->world_cam[3]),
			diff = cursorLoc - markerLoc;
		float dist = glm::dot(diff, diff);
		if (dist < leastDist) {
			leastDist = dist;
			leastInd = i;
			leastIsPose = true;
		}
	}

	//if found
	if (leastInd >= 0) {
		std::string objName = "";

		if (leastIsPose) {
			//if found parent
			objName = poses[leastInd]->name;
		} else {
			//found obj world
			objName = objs[leastInd]->name;
		}

		printf("%s is %f cm away from %s\n", name.c_str(), sqrt(leastDist), objName.c_str());

	}
	
	*/

	//green button goes to s_apps
	//if (green.press()) {
	//	state = s_apps;
	//	renderState();
	//	//enable the blue button
	//	blue.swapTextures();
	//}
	//blue button does nothing
}
void HandheldInput::process_place() {
	/*
	//blue button places app
	if (blue.press()) {
		//place the app into the scene
		lastBind = BIND_PURGATORY;
		state = s_idle;
		//disable blue during idle
		blue.swapTextures();
		//make it not glow anymore
		blue.press();
	}
	//slide to right scrolls binding		->	hud		personal	parent	world	->
	if (blue.slideL2R()) {
		manualBinding = true;
		if (lastBind == BIND_HUD) lastBind = BIND_PERSONAL;
		else if (lastBind == BIND_PERSONAL) lastBind = BIND_PARENT;
		else if (lastBind == BIND_PARENT) lastBind = BIND_WORLD;
		else if (lastBind == BIND_WORLD) lastBind = BIND_HUD;
	}
	//slide to left scrolls bindings		<-	hud		personal	parent	world	<-
	if (blue.slideR2L()) {
		manualBinding = true;
		if (lastBind == BIND_HUD) lastBind = BIND_WORLD;
		else if (lastBind == BIND_PERSONAL) lastBind = BIND_HUD;
		else if (lastBind == BIND_PARENT) lastBind = BIND_PERSONAL;
		else if (lastBind == BIND_WORLD) lastBind = BIND_PARENT;
	}
	//green button goes back to s_apps
	if (green.press()) {
		lastBind = BIND_PURGATORY;
		state = s_apps;
	}
	//figure out which context to bind to
	if (isfacing && isapproaching && isclose) curBind = BIND_HUD;
	else if (isfacing && isretreating && isfar) curBind = BIND_PERSONAL;
	else if (!isfacing && isresting) {
		//look for close objects
			//change ray geometry to point to parent
		//else
		curBind = BIND_WORLD;
	}
	//the state can be 'sticky' so seperate here
	if (curBind == BIND_WORLD) traceDepth();
	//always? or just on a binding change
	if (curBind != lastBind) {
		lastBind = curBind;
		renderState();
	}
	*/
}
void HandheldInput::process_apps() {
	/*
	//blue button selects app
	if (blue.press()) {
		state = s_place;
		renderState();
	}
	//slide to left scrolls apps
	if (blue.slideR2L()) {
		printf("left\n");
		renderState();
	}
	//slide to right scrolls apps
	if (blue.slideL2R()) {
		printf("right\n");
		renderState();
	}
	//green button goes to s_idle
	if (green.press()) {
		state = s_idle;
		renderState();
		blue.swapTextures();
	}
	*/
}

//usually gets called on state transitions
void HandheldInput::renderState() {
	if (state == s_idle) {
		//pretty much nothing goes on here
		bgs[b_idle].copyTo(screen);
		//figure out if we're close to objects to edit
		//render them in a window with info :D !!!
	} else if (state == s_apps) {
		//titled background
		bgs[b_title].copyTo(screen);
		//title text
		cv::putText(screen, "Apps", cv::Point(32, 22), cv::HersheyFonts::FONT_HERSHEY_PLAIN, 1.5, cv::Scalar(255, 128, 0), 2);
		//selection box
		cv::rectangle(screen, cv::Rect(31, 41, 66, 66), cv::Scalar(128, 64, 0), 1);
		cv::rectangle(screen, cv::Rect(32, 42, 64, 64), cv::Scalar(255, 128, 0), 1);
		//render programs
		AlphaBlend(pgmICOs[p_symbol], screen, cv::Point(32, 42), cv::Point(4, 4));
		AlphaBlend(pgmICOs[p_firefox], screen, cv::Point(98, 42), cv::Point(4, 4));
	} else if (state == s_place) {
		//if possible binding has changed upload new binding pic
		bindICOs[curBind].copyTo(screen);
		
		//draw the program icon
		if (curBind == BIND_PURGATORY) AlphaBlend(pgmICOs[p_symbol], screen, cv::Point(32, 42), cv::Point(4, 4));
	}

	uploadTexture(screen, cont.glTex[t_active]);
}

//must implement for scene functionality
void HandheldInput::update() {
	//blue.update();
	//green.update();

	//try update (also throttled)
	if (hasOcclusion()) i_OcclusionFeedback::update();
	else time++;
	//throttle button logic
	if ((time & 3) != 0) return;

	//get translation factor
	float tx = cont.view_cam[3].x, ty = cont.view_cam[3].y, tz = cont.view_cam[3].z,
		  //as tr approaches infinity, the view vector approaches parallel with marker normal
		  tr = ty / sqrt(tx*tx + tz*tz + 0.0001f);

	//state update
	lastDist = distance;
	lastDiff = distDiff;
	distance = sqrt(tx*tx + ty*ty + tz*tz);
	distDiff = lastDist - distance;
	//some boolean logic
	isfacing = (tr > 2.f);
	isclose = (distance - znear < 20.f);
	isfar = (reach - distance < 20.f);
	isapproaching = (distDiff > 1.f);
	isretreating = (distDiff < -1.f);
	isresting = (!isapproaching && !isretreating);

	//check state
	if (state == s_idle) {
		process_idle();
	} else if (state == s_place) {
		process_place();
	} else if (state == s_apps) {
		process_apps();
	}

	traceDepth();

}
void HandheldInput::render() {
	if (!_geometry || !_scene || !_textures || !_shade) {
		printf("Handheld: no %s%s%s%s\n", _geometry ? "" : "geometry ", _scene ? "" : "scene ", _textures ? "" : "textures ", _shade ? "" : "shaders ");
		return;
	}

	if (hasOcclusion()) {
		occont.model = cont.model;
		occont.view = cont.view;
	}

	//aliasing saves keystrokes and cpu cycles
	Shader& shade = *cont.shade;
	Context& txt = *cont.target;

	glDisable(GL_DEPTH_TEST);

	glUseProgram(shade.getID());
	glBindVertexArray(cont.glVAO[a_screen]);

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
	
	// Draw the square!
	glDrawElements(GL_TRIANGLES, cont.n_inds, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glDisable(cont.glTex[t_active]);
	glEnable(GL_DEPTH_TEST);

	// Draw the laser with laser shade
	//if (state == s_place && lastBind == BIND_WORLD) {
		glUseProgram(laserShade.getID());
		glBindVertexArray(cont.glVAO[a_laser]);

#ifdef BUILD_GLASSES
		glUniform2f(shade.uniform("frameSize"), 2.f / txt.viewp[2], 1.f / txt.viewp[3]);
#else
		glUniform2f(laserShade.uniform("frameSize"), 1.f / txt.viewp[2], 1.f / txt.viewp[3]);
#endif

		glUniformMatrix4fv(laserShade.uniform("MVP"), 1, GL_FALSE, &mvp[0][0]);
		glUniformMatrix4fv(laserShade.uniform("MV"), 1, GL_FALSE, &mv[0][0]);
		glUniformMatrix4fv(laserShade.uniform("M"), 1, GL_FALSE, &cont.model[0][0]);
		glUniformMatrix4fv(laserShade.uniform("normalMatrix"), 1, GL_FALSE, &normal[0][0]);

		//bind texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cont.glTex[t_laser]);
		glUniform1i(laserShade.uniform("texture"), 0);

		// Draw the laser!
		glDrawElements(GL_TRIANGLES, cont.n_inds, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		glDisable(cont.glTex[t_laser]);
	//}

	//manually inherit pose
	//blue.cont.view = cont.view;
	//blue.cont.view_cam = cont.view_cam;
	//blue.render();

	//green.cont.view = cont.view;
	//green.cont.view_cam = cont.view_cam;
	//green.render();
}
