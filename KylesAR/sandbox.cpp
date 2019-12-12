#include "sandbox.h"

Sandbox::Sandbox(std::string nom, BINDINGS _binding, BIND_BEHAVIOR _behavior, int pID) : i_SceneObject() {
	name = nom;
	rayMode = 2;
	info.binding = _binding;
	info.behavior = _behavior;
	info.desiredPoseID = pID;
	_scene = true;
}

//set up render pass
void Sandbox::setup(const Context &txt, bool hasDepth) {
	//raytrace uniforms
	std::vector<std::string> rayUniforms = { "mode", "time", "fov", "frameSize", "camLoc", "camPose" };
	if (hasDepth) {
		//depth uniforms
		std::vector<std::string> depthUniforms = { "depthFrame", "depthSize" };
		rayUniforms.insert(rayUniforms.end(), depthUniforms.begin(), depthUniforms.end());
		//the shader that renders ray based content to the framebuffer
		rayShade = Shader("glsl/raytrace/ray.depth.vert", "glsl/raytrace/ray.depth.frag");
	} else {
		//the shader that renders ray technologies to the framebuffer
		rayShade = Shader("glsl/raytrace/ray.vert", "glsl/raytrace/ray.frag");
	}
	//add bulk of ray uniforms
	rayShade.addUniforms(rayUniforms);
	//give pointer to contnet
	cont.shade = &rayShade;

	//set up the render pass
	createFullScreenPassNoDepth(cont, 0, txt.viewp[2], txt.viewp[3], GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);

	_geometry = true;
}


//change raytrace mode in shader
void Sandbox::changeMode(int mode) {
	if (mode < 0) rayMode = (rayMode + 1) % 3;
	else rayMode = mode;
}


void Sandbox::update() {

}

void Sandbox::render() {
	glBindFramebuffer(GL_FRAMEBUFFER, cont.glFBO[f_ray]);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use raytrace shader
	Shader &shade = *cont.shade;
	Context &txt = *cont.target;

	glUseProgram(shade.getID());

	//use vao
	glBindVertexArray(cont.glVAO[a_quad]);

	//some uniforms...
	glUniform1i(shade.uniform("mode"), rayMode);
	glUniform1i(shade.uniform("time"), txt.frameNumber);
	glUniform1f(shade.uniform("fov"), 1.f / tan(txt.fov * DEG2RAD * 0.5f));

#ifdef BUILD_GLASSES	
	glUniform2f(shade.uniform("frameSize"), 2.f / txt.viewp[2], 1.f / txt.viewp[3]);
#else
	glUniform2f(shade.uniform("frameSize"), 1.f / txt.viewp[2], 1.f / txt.viewp[3]);
#endif

	//if uniform exists
	if (shade.uniform("depthFrame")) {
		//bind texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, txt.resID[txt.r_depth]);
		glUniform1i(shade.uniform("depthFrame"), 0);
	}

	//get the camera matrix for the raytrace pipeline
	eyePose = glm::mat3(cont.view_cam);
	eyeLoc = glm::vec3(cont.view_cam[3]);

	glUniformMatrix3fv(shade.uniform("camPose"), 1, GL_FALSE, &eyePose[0][0]);
	glUniform3fv(shade.uniform("camLoc"), 1, &eyeLoc[0]);

	//dont have light tracking interface worked out yet
	//glUniform1f(shade.uniform("light_power"), ((Fiducial *)trackers[i])->smpHi * 0.002f * ((Fiducial *)trackers[i])->lightPower);
	//glUniform3f(shade.uniform("light_color"), trackers[i]->whiteBal.r * 0.007, trackers[i]->whiteBal.g * 0.007, trackers[i]->whiteBal.b * 0.007);
	//glUniform3f(shade.uniform("light_cameraspace"), trackers[i]->light.x, trackers[i]->light.y, trackers[i]->light.z);

	glDrawArrays(GL_TRIANGLES, 0, sizeof(quad_verts));

	glBindVertexArray(0);
	if (shade.uniform("depthFrame")) glDisable(txt.resID[txt.r_depth]);
}
