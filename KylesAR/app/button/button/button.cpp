#include "stdafx.h"
#include "button.h"

Button::Button(std::string nom) : i_SceneObject() {
	name = nom;
	cont.model = glm::mat4(1.f);
	cont.view = glm::mat4(1.f);
	cont.view_cam = glm::mat4(1.f);
	cont.normalMatrix = glm::mat4(1.f);
	hold = 0;
	timeout = 30;
}
//scene graphical init
Button::Button(std::string nom, BINDINGS _binding, BIND_BEHAVIOR _behavior, int pID) : Button(nom) {
	name = nom;
	cont.model = glm::mat4(1.f);
	cont.view = glm::mat4(1.f);
	cont.view_cam = glm::mat4(1.f);
	cont.normalMatrix = glm::mat4(1.f);
	hold = 0;
	timeout = 30;
	info.binding = _binding;
	info.behavior = _behavior;
	info.desiredPoseID = pID;
	_scene = true;
}

//moar init
void Button::setup_textures(const cv::Mat& active, const cv::Mat& inactive) {
	active.copyTo(tex_active);
	cont.glTex[t_active] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	uploadTexture(tex_active, cont.glTex[t_active]);

	inactive.copyTo(tex_inactive);
	cont.glTex[t_inactive] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE);
	uploadTexture(tex_inactive, cont.glTex[t_inactive]);

	//success!
	_textures = true;
}
//even moar init
void Button::setup_occlusion(int oc_width, int oc_height, Shader* _ocShade) {
	if (oc_width * oc_height <= 0) {
		printf("Button: %s setup_occlusion() fail (w:%i h:%i)\n", name, oc_width, oc_height);
		return;
	}
	i_OcclusionFeedback::setup_occlusion(oc_width, oc_height, _ocShade);
}
//moar init
void Button::setup_geometry() {
	i_OcclusionFeedback::setup_geometry(button_verts, button_norms, button_uvs, button_inds, 4, 6);

	cont.verts = button_verts;
	cont.norms = button_norms;
	cont.uvs = button_uvs;
	cont.inds = button_inds;
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

	_geometry = true;
}
//
void Button::setup_shader() {
	// load shaders
	std::string path = getCWD();

	//shared uniforms between shaders
	std::vector<std::string> buttonUniforms = { "texture", "frameSize", "MVP", "MV", "M", "normalMatrix", "light_power", "light_color", "light_cameraspace", "brightness" };
	std::string vertPath = path + std::string("\\glsl\\button.vert"),
				fragPath = path + std::string("\\glsl\\button.frag");

	buttonShade = Shader(vertPath, fragPath);
	if (buttonShade.getID() == NULL) return;

	//add all the uniforms
	buttonShade.addUniforms(buttonUniforms);

	//use as main shader
	cont.shade = &buttonShade;
}

//press like a normal button
bool Button::press() {
	if (isIntersected())
		hold += int(hold < delay);
	else
		hold -= int(hold > 0);
	if (hold > (delay >> 1))
		cooldown += int(cooldown < delay);
	else if (hold == 0) {
		cooldown = 0;
		pressing = false;
	}
	if (!pressing)
		brightness = 1.0f - float(cooldown) / float(delay * 2);
	if (cooldown == delay) {
		cooldown = 0;
		if (!pressing) {
			pressing = true;
			return true;
		}
	}
	return false;
}
//when something is occluding
bool Button::hover() {
	if (isOccluded())
		hover_hold += int(hover_hold < hover_delay);
	else
		hover_hold -= int(hover_hold > 0);
	if (hover_hold > (hover_delay >> 1))
		hover_cooldown += int(hover_cooldown < hover_delay);
	else if (hover_hold == 0) {
		hover_cooldown = 0;
		hovering = false;
	}
	if (hover_cooldown == hover_delay) {
		hover_cooldown = 0;
		if (!hovering) {
			hovering = true;
			return true;
		}
	}
	return false;
}
//when the surface has been slid/swiped over left to right
bool Button::slideL2R() {
	//if no touch data
	if (!isIntersected())
		slideLR_hold -= int(slideLR_hold > 0); 
	else if (isRecent()) {
		//if we are moving the right way
		if (lastSingle.x < curSingle.x)
			slideLR_hold += int(slideLR_hold < slide_delay);
		else if (lastSingle.x > curSingle.x)
			slideLR_hold -= int(slideLR_hold > 0);
		//if hold is great enough
		if (slideLR_hold == slide_delay) {
			slideLR_hold = 0;
			return true;
		}
	}
	return false;
}
//when the surface has been slid/swiped over right to left
bool Button::slideR2L() {
	//if no touch data
	if (!isIntersected())
		slideRL_hold -= int(slideRL_hold > 0);
	else if (isRecent()) {
		//if we are moving the wrong way
		if (lastSingle.x < curSingle.x)
			slideRL_hold -= int(slideRL_hold > 0);
		else if (lastSingle.x > curSingle.x)
			slideRL_hold += int(slideRL_hold < slide_delay);
		//if hold is great enough
		if (slideRL_hold == slide_delay) {
			slideRL_hold = 0;
			return true;
		}
	}
	return false;
}

//callbacks
void Button::sceneCallback(Scene* scene, Context* renderer) {
	i_OcclusionFeedback::sceneCallback(scene, renderer);
	info.parent = scene;
	cont.target = renderer;
	cont.model[3][2] = -15.f;
	int bwidth = int(ceilf(button_verts[6] * 20.f)), bheight = int(ceilf(button_verts[8] * 20.f));
	setup_occlusion(bwidth, bheight, nullptr);
}
//bind to *parent* callback
void Button::bindCallback(PositionObject* parent) {
	if (parent != nullptr)
		cont.model[0][3] = parent->size;
}
//remove from scene callback
void Button::removeCallback() {}

//switches texture ID's
void Button::swapTextures() {
	GLuint temp = cont.glTex[t_inactive];
	cont.glTex[t_inactive] = cont.glTex[t_active];
	cont.glTex[t_active] = temp;
}

//called every frame it's visible
void Button::update() {
	//try update (also throttled)
	if (hasOcclusion()) i_OcclusionFeedback::update();
	else time++;
	//throttle button logic
	if ((time & 3) != 0) return;

	//act like a button
	if (press()) swapTextures();
}
//should add to batch process or render individually
void Button::render() {
	if (!_textures || !_geometry) {
		printf("Button %s no%s%s\n", name.c_str(), _textures ? " " : " textures ", _geometry ? " " : " geometry ");
		return;
	}

	if (cont.shade == nullptr || cont.target == nullptr) {
		printf("Button%s%snot set up\n", cont.shade == nullptr ? " shade " : " ", cont.target == nullptr ? " target " : " ");
		return;
	}

	if (hasOcclusion()) {
		occont.model = cont.model;
		occont.view = cont.view;
	}

	//aliasing saves keystrokes and cpu cycles
	Shader& shade = *cont.shade;
	Context& txt = *cont.target;

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
	//brightness (pretty much gamma)
	glUniform1f(shade.uniform("brightness"), brightness);

	//bind texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cont.glTex[t_inactive]);
	glUniform1i(shade.uniform("texture"), 0);

	// Draw the square!
	glDrawElements(GL_TRIANGLES, cont.n_inds, GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
	glDisable(cont.glTex[t_inactive]);
}
