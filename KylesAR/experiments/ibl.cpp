#include "ibl.h"

namespace IBL {
	// psudo random
	glm::vec3 hash33(glm::vec3 p) {
		glm::vec3 h = glm::vec3(glm::dot(p, glm::vec3(467.127f, 881.311f, 571.521f)),
								glm::dot(p, glm::vec3(627.101f, 311.683f, 913.331f)),
								glm::dot(p, glm::vec3(783.331f, 561.101f, 323.127f)));
		return glm::fract(glm::sin(h) * 467.281f);
	}
	// generate direction
	glm::vec3 rndDirection(glm::vec3 chaos) {
		glm::vec3 r0 = hash33(chaos + 17.07f),
				  r1 = hash33(chaos - 13.11f);
		return glm::normalize(r0 + r1 - 1.f);
	}
	// generate orientation
	glm::mat3 rndOrientation(glm::vec3 chaos) {
		glm::vec3 chao2 = chaos.zxy,
			r = rndDirection(chaos),
			u = rndDirection(chao2 + 7.31f),
			f = glm::normalize(glm::cross(r, u));
		u = glm::normalize(glm::cross(r, f));
		return glm::mat3(r, u, f);
	}
	// setup and render
	int renderScene(Context *ctx, i_Camera *icam) {
		//load up equirectangular textures 1-12
		cv::Mat env[] = {
			cv::imread("images/1.jpg"),
			cv::imread("images/2.jpg"),
			cv::imread("images/3.jpg"),
			cv::imread("images/4.jpg"),
			cv::imread("images/5.jpg"),
			cv::imread("images/6.jpg"),
			cv::imread("images/7.jpg"),
			cv::imread("images/8.jpg"),
			cv::imread("images/9.jpg"),
			cv::imread("images/10.jpg"),
			cv::imread("images/11.jpg"),
			cv::imread("images/12.jpg"),
		};

		cv::waitKey(100);

		cv::Mat canImg = cv::imread("images/can.jpg"),
			metalImg = cv::imread("images/metal.jpg");

		cv::waitKey(100);

		iblCont.glTex[0] = createTexture(GL_LINEAR, GL_LINEAR, GL_REPEAT, false);
		uploadTexture(canImg, iblCont.glTex[0]);
		glBindTexture(GL_TEXTURE_2D, 0);
		iblCont.glTex[1] = createTexture(GL_LINEAR, GL_LINEAR, GL_REPEAT, false);
		uploadTexture(metalImg, iblCont.glTex[1]);
		glBindTexture(GL_TEXTURE_2D, 0);
		iblCont.glTex[2] = createTexture(GL_LINEAR, GL_LINEAR, GL_REPEAT, false);
		uploadTexture(env[0], iblCont.glTex[2]);
		glBindTexture(GL_TEXTURE_2D, 0);

		printf("loaded textures\n");

		//IBL shader
		vector<string> iblUniforms = { "canTex", "metalTex", "environment", "NM", "VP", "V" };
		Shader iblShade = Shader("experiments\\glsl\\ibl.vert", "experiments\\glsl\\ibl.frag");
		iblShade.addUniforms(iblUniforms);

		printf("loaded shaders\n");

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		glClearColor(.5, .5, .5, 1.);

		//set up OpenGL and viewport to render images with icam parameters
		ctx->viewp = glm::vec4(0, 0, icam->rgb.width, icam->rgb.height);
		ctx->projection = glm::perspective(icam->rgb.fovV, (float)icam->rgb.width / (float)icam->rgb.height, .1f, 1000.f);
		glViewport(0, 0, icam->rgb.width, icam->rgb.height);

		//render texture
		iblCont.glRenTex[0] = createTexture(GL_LINEAR, GL_LINEAR, GL_CLAMP, false);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, icam->rgb.width, icam->rgb.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		//depth attachment
		iblCont.glRenTex[1] = createTexture(GL_LINEAR, GL_LINEAR, GL_CLAMP, false);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, icam->rgb.width, icam->rgb.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		//frame buffer
		iblCont.glFBO[0] = createFrameBuffer(iblCont.glRenTex[0], iblCont.glRenTex[1], false);
		//best check ever
		GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ||
			status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ||
			status == GL_FRAMEBUFFER_UNSUPPORTED) {
			printf("Failed to create fullscreen pass\n");
		}
		//unbind frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		printf("created RT and FBO\n");

		//indexed geometry
		iblCont.verts = can.coordinates;
		iblCont.norms = can.normals;
		iblCont.inds = can.indices;
		iblCont.uvs = can.texcoords;
		iblCont.n_verts = can.nVertices;
		iblCont.n_inds = can.nIndices;

		//vert array
		iblCont.glVAO[0] = createVertexArray(false);
		//vert buffer
		iblCont.glVBO[0] = createBuffer();
		uploadBuffer(GL_ARRAY_BUFFER, iblCont.verts, iblCont.n_verts * 3 * sizeof(float), iblCont.glVBO[0], false);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		//norm buffer
		iblCont.glVBO[1] = createBuffer();
		uploadBuffer(GL_ARRAY_BUFFER, iblCont.norms, iblCont.n_verts * 3 * sizeof(float), iblCont.glVBO[1], false);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
		//uv buffer
		iblCont.glVBO[2] = createBuffer();
		uploadBuffer(GL_ARRAY_BUFFER, iblCont.uvs, iblCont.n_verts * 2 * sizeof(float), iblCont.glVBO[2], false);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
		//texture index buffer
		iblCont.glVBO[3] = createBuffer();
		uploadBuffer(GL_ARRAY_BUFFER, can.matindices, iblCont.n_verts * sizeof(unsigned char), iblCont.glVBO[3], false);
		glEnableVertexAttribArray(3);
		glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, GL_FALSE, 0);
		//indices
		iblCont.glVBO[4] = createBuffer();
		uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, iblCont.inds, iblCont.n_inds * sizeof(unsigned int), iblCont.glVBO[4], false);
		//and we're done!
		glBindVertexArray(0);

		printf("created buffers\n");

		//render this stuff
		const int FRAMES = 256, BATCH = 12;
		const float MAX_DIST = 130.f,
			MIN_DIST = 13.f;

		//render to FBO
		glBindFramebuffer(GL_FRAMEBUFFER, iblCont.glFBO[0]);

		glUseProgram(iblShade.getID());
		glBindVertexArray(iblCont.glVAO[0]);

		//bind textures
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, iblCont.glTex[0]);
		glUniform1i(iblShade.uniform("canTex"), 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, iblCont.glTex[1]);
		glUniform1i(iblShade.uniform("metalTex"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, iblCont.glTex[2]);
		glUniform1i(iblShade.uniform("environment"), 2);

		//each batch gets another environment
		//for (int j = 0; j < BATCH; j++) {

		//printf("batch number:%i\n", j);

		//generate N images in batches
		//for (int i = 0; i < FRAMES; i++) {

		//printf("frame number:%i\n", i);

		//clear the FBO as this is the first write of a frame
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//random pose
		//glm::vec3 seed = glm::vec3(float(i + j*FRAMES - rand()), float(-MIN_DIST*i - rand()), float(rand() - j + i)),
		//		  loc = hash33(seed) * MAX_DIST;
		//glm::mat3 orient = rndOrientation(loc);
		//glm::mat4 v = glm::mat4(glm::vec4(orient[0], 0.),
		//						glm::vec4(orient[1], 0.),
		//						glm::vec4(orient[2], 0.),
		//						glm::vec4(loc,		 1.)),
		//		  vp = ctx->projection * v,
		//		  normal = glm::inverse(glm::transpose(v));

		glm::mat4 v = glm::mat4(1.f, 0.f, 0.f, 0.f,
			0.f, -1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			0.f, 6.f, -30.f, 1.f),
			vp = ctx->projection * v,
			normal = glm::inverse(glm::transpose(v));

		glUniformMatrix4fv(iblShade.uniform("NM"), 1, GL_FALSE, &normal[0][0]);
		glUniformMatrix4fv(iblShade.uniform("VP"), 1, GL_FALSE, &vp[0][0]);
		glUniformMatrix4fv(iblShade.uniform("V"), 1, GL_FALSE, &v[0][0]);

		// Draw the can!
		glDrawElements(GL_TRIANGLES, iblCont.n_inds * 3, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
		glDisable(iblCont.glTex[0]);
		glDisable(iblCont.glTex[1]);
		glDisable(iblCont.glTex[2]);
		//}

		printf("rendered frames\n");

		//save images in large batches with poses
		cv::Mat result = cv::Mat::zeros(icam->rgb.height, icam->rgb.width, CV_8UC4);
		cv::waitKey(100);
		downloadTexture(result, iblCont.glRenTex[0]);
		cv::waitKey(100);
		cv::imshow("frame", result);
		cv::waitKey(100);
		//}

		printf("done\n");
		return iblCont.glRenTex[0];
	}
}
