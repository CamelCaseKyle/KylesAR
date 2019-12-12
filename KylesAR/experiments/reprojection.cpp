#include "reprojection.h"

Reprojection::Reprojection(Context *ctx, i_Camera *icam, FreespaceIMU *fimu) {
	renderer = ctx;
	camera = icam;
	imu = fimu;

	// declare our fullscreen quad
	createFullScreenPassNoDepth(reprojCont, 0, icam->rgb.width, icam->rgb.height, GL_NEAREST, GL_NEAREST, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);

	//// now load up the dewarp mesh
	//reproj_pass2Cont.glVAO[0] = createVertexArray(false);
	//
	////distortion quad verts
	//reproj_pass2Cont.glVBO[0] = createBuffer();
	//uploadBuffer(GL_ARRAY_BUFFER, &ctx->dewarp.ndCoords[0], ctx->dewarp.coord_size, reproj_pass2Cont.glVBO[0], false);
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	////texture coords
	//reproj_pass2Cont.glVBO[1] = createBuffer();
	//uploadBuffer(GL_ARRAY_BUFFER, &ctx->dewarp.txCoords[0], ctx->dewarp.tx_size, reproj_pass2Cont.glVBO[1], false);
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	////indices
	//reproj_pass2Cont.glVBO[2] = createBuffer();
	//uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, &ctx->dewarp.inds[0], ctx->dewarp.ind_size, reproj_pass2Cont.glVBO[2], false);
	//
	//glBindVertexArray(0);

	// load reprojection shader
	std::vector<std::string> reprojUniforms = { "frame", "MVP" };
	reprojShade = Shader("experiments\\glsl\\reproj.vert", "experiments\\glsl\\reproj.frag");
	reprojShade.addUniforms(reprojUniforms);
	//reprojToScreen = Shader("experiments\\glsl\\reproj_pass2.vert", "experiments\\glsl\\reproj_pass2.frag");
	//reprojToScreen.addUniforms({"frame"});

	// the fbo to reproject
	fbo = IBL::renderScene(renderer, camera);

}

void Reprojection::render() {

	// my IMU test
	timePoint lastTime = imu->sampleTime;
	imu->sample();

	glDisable(GL_CULL_FACE);
	// render to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use passthrough shader
	glUseProgram(reprojShade.getID());

	// load our fullscreen quad
	glBindVertexArray(reprojCont.glVAO[0]);
	//bind textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fbo); //renderer->textures[renderer->t_frame]
	glUniform1i(reprojShade.uniform("frame"), 0);

	// calculate the rotation difference here
	glm::mat4 mv = glm::eulerAngleYXZ(imu->angVel.y * .1f, imu->angVel.x * -.1f, 0.f), //accelnograv
		mvp = mv * glm::translate(glm::vec3(0., 0., 1.)); //renderer->projection

	glUniformMatrix4fv(reprojShade.uniform("MVP"), 1, GL_FALSE, &mvp[0][0]);

	// First draw pass, project / rotate the rendered image
	glDrawArrays(GL_TRIANGLES, 0, reprojCont.n_verts);

	glBindVertexArray(0);
	glDisable(fbo);

	glEnable(GL_CULL_FACE);

	return;

	// render to FBO
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// clear it
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use passthrough shader
	glUseProgram(reprojToScreen.getID());

	// load our distortion mesh
	glBindVertexArray(reproj_pass2Cont.glVAO[0]);
	// bind textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, reproj_pass2Cont.glFBO[0]);
	glUniform1i(reprojToScreen.uniform("frame"), 0);

	// draw the distortion mesh
	glDrawElements(GL_TRIANGLES, GLsizei(renderer->dewarp.inds.size()), GL_UNSIGNED_INT, 0);

	//glBindVertexArray(0);
	glDisable(reproj_pass2Cont.glFBO[0]);

}
