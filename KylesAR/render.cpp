#include "render.h"

//fill content with an unindexed fullscreen quad and set up drawing to it. RT and FBO slots <slot> and RT slot <NUM_RT - slot - 1> are used for color and depth
void createFullScreenPass(Content &cont, int slot, GLsizei width, GLsizei height, GLuint mag, GLuint min, GLint itlf, GLenum fmt, GLenum typ) {
	//render texture
	cont.glRenTex[slot] = createTexture(mag, min, GL_CLAMP, false);
	glTexImage2D(GL_TEXTURE_2D, 0, itlf, width, height, 0, fmt, typ, 0);
	//depth attachment
	cont.glRenTex[NUM_RT - slot - 1] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP, false);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	//frame buffer
	cont.glFBO[slot] = createFrameBuffer(cont.glRenTex[slot], cont.glRenTex[NUM_RT - slot - 1], false);
	//best check ever
	GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ||
		status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ||
		status == GL_FRAMEBUFFER_UNSUPPORTED) {
		printf("Failed to create fullscreen pass no depth\n");
	}
	//unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//unindexed geometry
	cont.verts = quad_verts;
	cont.n_verts = 6;
	//vert array
	cont.glVAO[0] = createVertexArray(false);
	//vert buffer
	cont.glVBO[0] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &cont.verts[0], cont.n_verts * 3 * sizeof(float), cont.glVBO[0], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//and we're done!
	glBindVertexArray(0);
}
//fill content with an unindexed fullscreen quad and set up drawing to it, but no depth buffer
void createFullScreenPassNoDepth(Content &cont, int slot, GLsizei width, GLsizei height, GLuint mag, GLuint min, GLint itlf, GLenum fmt, GLenum typ) {
	//render texture
	cont.glRenTex[slot] = createTexture(mag, min, GL_CLAMP, false);
	glTexImage2D(GL_TEXTURE_2D, 0, itlf, width, height, 0, fmt, typ, 0);
	//frame buffer
	cont.glFBO[slot] = createFrameBuffer(cont.glRenTex[slot], false);
	//best check ever
	GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ||
		status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ||
		status == GL_FRAMEBUFFER_UNSUPPORTED) {
		printf("Failed to create fullscreen pass no depth\n");
	}
	//unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//unindexed geometry
	cont.verts = quad_verts;
	cont.n_verts = 6;
	//vert array
	cont.glVAO[0] = createVertexArray(false);
	//vert buffer
	cont.glVBO[0] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &cont.verts[0], cont.n_verts * 3 * sizeof(float), cont.glVBO[0], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	//and we're done!
	glBindVertexArray(0);
}
//creates a texture on the GPU
GLuint createTexture(GLuint mag, GLuint min, GLuint wrap, bool unbind) {
	GLuint newID;
	glGenTextures(1, &newID);
	glBindTexture(GL_TEXTURE_2D, newID);
	//parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
	//if should leave bound
	if (unbind) glBindTexture(GL_TEXTURE_2D, 0);
	return newID;
}

//creates a basic framebuffer with depth
GLuint createFrameBuffer(GLuint color_attachment, GLuint depth_attachment, bool unbind) {
	GLuint newFBO;
	//The raytrace frame buffer
	glGenFramebuffers(1, &newFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, newFBO);
	//attach vertex render textures
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_attachment, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_attachment, 0);
	//tell it what to do with the buffers
	GLenum drawBuffers[2] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
	glDrawBuffers(2, drawBuffers); // "2" is the size of DrawBuffers
	if (unbind) glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return newFBO;
}
//creates a basic framebuffer
GLuint createFrameBuffer(GLuint color_attachment, bool unbind) {
	GLuint newFBO;
	//The raytrace frame buffer
	glGenFramebuffers(1, &newFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, newFBO);
	//attach vertex render textures
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_attachment, 0);
	//tell it what to do with the buffers
	GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);
	if (unbind) glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return newFBO;
}

//creates a buffer on the GPU
GLuint createBuffer() {
	GLuint newBuff;
	glGenBuffers(1, &newBuff);
	return newBuff;
}
//creates a basic framebuffer
GLuint createVertexArray(bool unbind) {
	GLuint newVAO;
	glGenVertexArrays(1, &newVAO);
	if (!unbind) glBindVertexArray(newVAO);
	return newVAO;
}

//uploads a texture to the GPU
void uploadTexture(cv::Mat &texture, GLuint textureID, bool generateMips, bool component) {
	std::vector<int> &info = textureHelper[texture.type()];
	glBindTexture(GL_TEXTURE_2D, textureID);
	//upload buffer
	glTexImage2D(GL_TEXTURE_2D, 0, component ? GL_DEPTH_COMPONENT : info[0], texture.cols, texture.rows, 0, info[1], info[2], texture.data);
	//generate the mips if requested
	if (generateMips) glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
}
//uploads a vertex array to the GPU
void uploadBuffer(GLuint type, const void *buffer, GLuint size, GLuint vboID, bool unbind) {
	glBindBuffer(type, vboID);
	glBufferData(type, size, buffer, GL_STATIC_DRAW);
	if (unbind) glBindBuffer(type, 0);
}

//get texture from GPU
void downloadTexture(cv::Mat &texture, GLuint textureID, bool unbind) {
	std::vector<int> &info = textureHelper[texture.type()];
	//bind texture in question
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureID);
	//download
	glGetTexImage(GL_TEXTURE_2D, 0, info[1], info[2], texture.data);
	if (unbind) glBindTexture(GL_TEXTURE_2D, 0);
}


//initialize GLFW, OpenGL, Dewarp, shaders
Context::Context(i_Camera *cam, char *title) : dewarp(cam->rgb.width >> 5) {
	
	// Initialise GLFW
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return;
	}

	//glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
#ifdef BUILD_GLASSES

	int count;
	bool found = false;
	GLFWmonitor** monitor = glfwGetMonitors(&count);
	//find glasses
	for (int i = 0; i < count; i++) {
		const GLFWvidmode *mode = glfwGetVideoMode(*monitor);
		if (mode->width == 2048) {
			found = true;
			break;
		}
		monitor++;
	}

	glfwWindowHint(GLFW_FLOATING, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	//create fullscreen window
	if (found) window = glfwCreateWindow(width * 2, height, title, *monitor, NULL);
	//create window with move/resize
	else window = glfwCreateWindow(width * 2, height, title, NULL, NULL);
	
#else

	glfwWindowHint(GLFW_FLOATING, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	
	window = glfwCreateWindow(cam->rgb.width, cam->rgb.height, title, NULL, NULL);

#endif

	if (window == nullptr) {
		printf("GLFW window == nullptr\n");
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		printf("GLEWinit != OK\n");
		name = nullptr;
		return;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	//section on screen
#ifdef BUILD_GLASSES
	viewp = glm::vec4(0, 0, width * 2, height);
#else
	viewp = glm::vec4(0, 0, cam->rgb.width, cam->rgb.height);
#endif

	//ratio
	aspect = float(cam->rgb.width) / float(cam->rgb.height);
	aspect_d = float(cam->depth.width) / float(cam->depth.height);
	//fov
	fov = cam->rgb.fovV;
	//actually calculate these
	fov_dh = cam->depth.fovH;
	fov_dv = cam->depth.fovV;

	// Projection matrix : fov and aspect from camera
	projection = glm::perspective(fov, aspect, .1f, 1000.f);
	depth_projection = glm::perspective(72.f, aspect, .1f, 1000.f);

	clearColor = glm::vec4(0.f, 0.f, 0.f, 0.f);
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);

	glViewport(0, 0, GLsizei(viewp[2]), GLsizei(viewp[3]));

	//proper occlusion
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	//speeds
	glEnable(GL_CULL_FACE);
	//point cloud rendering
	glEnable(GL_PROGRAM_POINT_SIZE);

	bool win = true;
	win &= initShaders(cam->depth.exists);
	win &= initFrameBuffer(cam->depth.exists, cam->depth.width, cam->depth.height);

	if (win) name = title;
	else printf("could not init shaders or framebuffer\n");

}

//init shaders
bool Context::initShaders(bool hasDepth) {
	//lists of uniforms
	std::vector<std::string> depthUniforms = { "depthFrame", "depthSize" };
	std::vector<std::string> blendUniforms = { "cameraFrame", "vertFrame", "rayFrame", "mode", "warpFactor", "frameSize" };
	std::vector<std::string> xformUniforms = { "depthCalib", "aspect", "scale", "PFD" };
	std::vector<std::string> augmentUniforms = { "texture", "frameSize", "MVP", "MV", "M", "normalMatrix", "light_power", "light_color", "light_cameraspace" };
	
	//always load
	passthroughShade = Shader("glsl\\renderer\\passthrough.vert", "glsl\\renderer\\passthrough.frag");
	passthroughShade.addUniforms({"tex", "texSize"});
	
	augmentShade = Shader("glsl\\renderer\\augment.vert", "glsl\\renderer\\augment.frag");
	augmentShade.addUniforms(augmentUniforms);
	
	if (hasDepth) {
		//transforms the depth map to align with color
		depthShade = Shader("glsl\\renderer\\transform.depth.vert", "glsl\\renderer\\transform.depth.frag");
		depthShade.addUniforms(depthUniforms);
		depthShade.addUniforms(xformUniforms);
		
		//the final shader that outputs desired content to the screen and handles distortion
		screenShade = Shader("glsl\\renderer\\blend.depth.vert", "glsl\\renderer\\blend.depth.frag");
		screenShade.addUniforms(blendUniforms);
		screenShade.addUniforms(depthUniforms);
	} else {

		//the final shader that outputs desired content to the screen and handles distortion
		screenShade = Shader("glsl\\renderer\\blend.vert", "glsl\\renderer\\blend.frag");
		screenShade.addUniforms(blendUniforms);
	}
	
	if (augmentShade.getID() == NULL || passthroughShade.getID() == NULL || screenShade.getID() == NULL) {
		printf("default shader(s) failed\n");
		return false;
	}

	shaders[s_augment] = &augmentShade;
	shaders[s_depth] = &depthShade;
	shaders[s_screen] = &screenShade;

	return true;
}

//initialize pipeline
bool Context::initFrameBuffer(bool hasDepth, int dw, int dh) {
	//scene quad's vertex array
	vao[a_quad] = createVertexArray(false);

	//distortion quad verts
	vbo[v_quad] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &dewarp.ndCoords[0], dewarp.coord_size, vbo[v_quad], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//texture coords
	vbo[v_quad_tx] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &dewarp.txCoords[0], dewarp.tx_size, vbo[v_quad_tx], false);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	//indices
	vbo[v_quad_ind] = createBuffer();
	uploadBuffer(GL_ELEMENT_ARRAY_BUFFER, &dewarp.inds[0], dewarp.ind_size, vbo[v_quad_ind], false);

	glBindVertexArray(0);

	//cam frame texture
	textures[t_frame] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP);
	//cam raw depth texture
	textures[t_depth] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP, false);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	//augmented render buffer
	resID[r_vert] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP, false);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, GLsizei(viewp[2]), GLsizei(viewp[3]), 0, GL_RGBA, GL_FLOAT, 0);
	//depth buffer component
	resID[r_depth] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP, false);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GLsizei(viewp[2]), GLsizei(viewp[3]), 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

	//The vertex frame buffer
	fbo[f_vertex] = createFrameBuffer(resID[r_vert], resID[r_depth], false);
	//best check ever
	GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT ||
		status == GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT ||
		status == GL_FRAMEBUFFER_UNSUPPORTED) {
		printf("Failed to generate vertex framebuffer %s: %i\n", name.c_str(), status);
		return false;
	}
	//unbind frame buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//load calib
	cv::Mat r200calib = cv::imread("images/r200depthCalib.png", CV_LOAD_IMAGE_UNCHANGED);
	textures[t_dcalib] = createTexture(GL_NEAREST, GL_NEAREST, GL_CLAMP);
	uploadTexture(r200calib, textures[t_dcalib]);

	//translate from identity
	fixDepth = glm::translate(fixDepth, glm::vec3(-10.5f, 0.f, 0.f));
	//store aspect correct template points to be transformed on gpu
	projection_verts = edgeDistanceNormalizedSTD(dw, dh);
	pro_vert_size = int(projection_verts.size()) * sizeof(projection_verts[0]);

	//create a VAO so we can recall this in 1 call
	vao[a_projection] = createVertexArray(false);

	//create and upload projection buffer
	vbo[v_projection] = createBuffer();
	uploadBuffer(GL_ARRAY_BUFFER, &projection_verts[0], pro_vert_size, vbo[v_projection], false);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindVertexArray(0);

	return true;
}

//set the clear color
void Context::ClearColor(glm::vec4 &newCol) {
	//keep track
	clearColor = newCol;
	//send to GL environment
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
}

//set window size
void Context::Reshape(float _aspect, float _fov) {
	//ratio
	aspect = _aspect;
	//view feild
	fov = _fov;
	//recalc projection
	projection = glm::perspective(fov, aspect, 50.0f, 400.0f);
}

//render an unindexed content VAO to its FBO with configurable texture bindings
void Context::RenderUnindexedContent(Content &cont, int fbo, int vao, std::vector<GLuint> textureUnits, std::vector<std::string> textureUniformNames, bool clear) {
	//render to fbo
	glBindFramebuffer(GL_FRAMEBUFFER, cont.glFBO[fbo]);
	//clear the fbo
	if (clear) glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//use our shader
	Shader &shade = *cont.shade;
	glUseProgram(shade.getID());
	//use our vao
	glBindVertexArray(cont.glVAO[vao]);

	//cache
	glm::mat4 mv = glm::mat4(1.);
	GLuint	uM = shade.uniform("M") + 1,
			uMV = shade.uniform("MV") + 1,
			uNM = shade.uniform("NM") + 1,
			uMVP = shade.uniform("MVP") + 1,
			frameSize = shade.uniform("frameSize") + 1;
	//some common uniforms
	if (frameSize) glUniform4f(frameSize - 1, viewp[2], viewp[3], 1.f / viewp[2], 1.f / viewp[3]);
	if (uM) glUniformMatrix4fv(uM - 1, 1, GL_FALSE, &cont.model[0][0]);
	if (uMV) {
		mv = cont.view * cont.model;
		glUniformMatrix4fv(uMV - 1, 1, GL_FALSE, &mv[0][0]);
	}
	if (uNM) {
		glm::mat4 normal = glm::inverse(glm::transpose(mv));
		glUniformMatrix4fv(uNM - 1, 1, GL_FALSE, &normal[0][0]);
	}
	if (uMVP) {
		glm::mat4 mvp = projection * mv;
		glUniformMatrix4fv(uMVP - 1, 1, GL_FALSE, &mvp[0][0]);
	}

	//bind texture i in texture unit i
	for (int i = 0; i < textureUnits.size(); i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, textureUnits[i]);
		glUniform1i(shade.uniform(textureUniformNames[i]), i);
	}

	// Draw the triangles
	glDrawArrays(GL_TRIANGLES, 0, cont.n_verts);
	glBindVertexArray(0);
}

//transform the depth image to align with color
void Context::transformDepth() {
	//render to depth component
	glBindFramebuffer(GL_FRAMEBUFFER, fbo[f_vertex]);
	
	//clear the FBO as this is the first write of a frame
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//use the transform shader
	Shader &shade = *shaders[s_depth];
	glUseProgram(shade.getID());

	//use the projection vertex array
	glBindVertexArray(vao[a_projection]);

	glm::mat4 pfd = depth_projection * fixDepth;

	//transform matrix
	glUniform1f(shade.uniform("aspect"), .75f); //rcp aspect of depth image
	glUniform1f(shade.uniform("scale"), 6500.f); //depth is mm converted from ushort [0-65535] to float [0-1]
	glUniformMatrix4fv(shade.uniform("PFD"), 1, GL_FALSE, &pfd[0][0]);

	//sensor data
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[t_depth]);
	glUniform1i(shade.uniform("depthFrame"), 0);
	//sensor correction
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, textures[t_dcalib]);
	glUniform1i(shade.uniform("depthCalib"), 1);

	// Draw the points!
	glDrawArrays(GL_POINTS, 0, pro_vert_size);

	glBindVertexArray(0);
	glDisable(textures[t_depth]);
	glDisable(textures[t_dcalib]);

}

//display everything on screen
void Context::display() {
	//render to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Use our shader
	Shader &shade = *shaders[s_screen];
	glUseProgram(shade.getID());

	//use vao
	glBindVertexArray(vao[a_quad]);

	//some uniforms...
	glUniform1i(shade.uniform("mode"), mode);
	glUniform1f(shade.uniform("warpFactor"), factor);
	glUniform2f(shade.uniform("bal"), eyebox.x, eyebox.y);

#ifdef BUILD_GLASSES	
	glUniform2f(shade.uniform("frameSize"), 2.f / viewp[2], 1.f / viewp[3]);
#else
	glUniform2f(shade.uniform("frameSize"), 1.f / viewp[2], 1.f / viewp[3]);
#endif

	//bind texture in Texture Unit 0
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[t_frame]);
	glUniform1i(shade.uniform("cameraFrame"), 0);
	
	glActiveTexture(GL_TEXTURE1);
	//if uniform exists
	if (shade.uniform("depthFrame")) {
		glBindTexture(GL_TEXTURE_2D, resID[r_depth]);
		//glBindTexture(GL_TEXTURE_2D, textures[t_depth]);
		glUniform1i(shade.uniform("depthFrame"), 1);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, resID[r_vert]);
		glUniform1i(shade.uniform("vertFrame"), 2);
	} else {
		glBindTexture(GL_TEXTURE_2D, resID[r_vert]);
		glUniform1i(shade.uniform("vertFrame"), 1);
	}

	// Draw the indexed triangles!
	glDrawElements(GL_TRIANGLES, GLsizei(dewarp.inds.size()), GL_UNSIGNED_INT, 0);
	
	glBindVertexArray(0);
	if (shade.uniform("depthFrame")) glDisable(resID[r_depth]);
	glDisable(resID[r_vert]);
	glDisable(textures[t_frame]);

	frameNumber++;
}
