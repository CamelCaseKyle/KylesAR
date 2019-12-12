/*
-dont use projection matrix on transform depth
-write content loader (obj file)
-rethink like a service, think effect composer
-seperate into render and stream contexts
-create and manage passes, let objects jump on other's passes
	-pre defined batch render passes:
		-augment vertex pass
		-occlusion feedback pass
		-screen pass

renderer
	render passes
		1.transform depth
		2.occlusion feedback
		
		a.augment & picking
		c.GUI
		d.dewarp
		e.screen

	render textures
		vert (depth testing vertex pipeline)
		depth (transformed depth point cloud)
		component (vert depth buffer)
		ray (depth saved to alpha for testing)
		picking

renderer.passes[renderer.p_augment].addContent()
renderer.passes[renderer.p_blend].draw()

*/

#pragma once

#define DBG_RENDER
//#define BUILD_GLASSES

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>

#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>

#include <glm.hpp>
#include <gtc\matrix_transform.hpp>

#include "rectExt.h"
#include "kibblesmath.h"
#include "dewarp.hpp"
#include "dev\i_Camera.h"
#include "shader.h"

#include <GL\glew.h>
#include <GLFW\glfw3.h>

//render modes
#define NUM_MODES	4

//forward declare here
struct Content;
class Context;

//unindexed fullscreen quad
static float quad_verts[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	1.0f, 1.0f, 0.0f
};

//fullscreen quad reversed
static float quad_verts_rev[] = {
	-1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
};

static std::unordered_map<int, std::vector<int>> textureHelper = {
	{ CV_8UC1, { GL_R8, GL_RED, GL_UNSIGNED_BYTE } },
	{ CV_8UC3, { GL_RGB, GL_BGR, GL_UNSIGNED_BYTE } },
	{ CV_8UC4, { GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE } },
	{ CV_16UC1, { GL_R16, GL_RED, GL_UNSIGNED_SHORT } },
	{ CV_32FC1, { GL_R32F, GL_RED, GL_FLOAT } }
};

//fill content with a fullscreen quad and set up drawing to it
void createFullScreenPass(Content &cont, int slot, GLsizei width, GLsizei height, GLuint mag = GL_NEAREST, GLuint min = GL_NEAREST, GLint itlf = GL_RGBA, GLenum fmt = GL_RGBA, GLenum typ = GL_UNSIGNED_BYTE);
//fill content with a fullscreen quad and set up drawing to it, but no depth buffer
void createFullScreenPassNoDepth(Content &cont, int slot, GLsizei width, GLsizei height, GLuint mag = GL_NEAREST, GLuint min = GL_NEAREST, GLint itlf = GL_RGBA, GLenum fmt = GL_RGBA, GLenum typ = GL_UNSIGNED_BYTE);
//create a texture, doesnt upload
GLuint createTexture(GLuint mag, GLuint min, GLuint wrap, bool unbind = true);
//create a FBO
GLuint createFrameBuffer(GLuint color_attachment, bool unbind = true);
GLuint createFrameBuffer(GLuint color_attachment, GLuint depth_attachment, bool unbind = true);
//create a VAO
GLuint createVertexArray(bool unbind = true);
//create buffer object, doesnt upload or bind anything
GLuint createBuffer();
//push data to GPU
void uploadTexture(cv::Mat &texture, GLuint textureID, bool generateMips = false, bool component = false);
//update geometry
void uploadBuffer(GLuint type, const void *buffer, GLuint size, GLuint vboID, bool unbind = true);
//pull data from GPU
void downloadTexture(cv::Mat &texture, GLuint textureID, bool unbind = true);

//this is a render context, todo: stream context
class Context {
private:
	//default built in shaders
	Shader augmentShade,
		   //transforms depth camera image
		   depthShade,
		   //renders everything to framebuffer
		   screenShade,
		   //just texturing passthrough
		   passthroughShade;
	//changing this alone wont do anything, need to use the method (same with aspect and FOV (except raytracer))
	glm::vec4 clearColor = glm::vec4(0.f, 0.f, 0.f, 0.f);
	
	//load built in shaders
	bool initShaders(bool hasDepth);
	//initialize environment and texture memory
	bool initFrameBuffer(bool hasDepth, int dw, int dh);

public:
	//index helpers
	const enum fboIDs	  { f_vertex };
	const enum rentexIDs  { r_vert, r_depth };
	const enum shaderIDs  { s_augment, s_depth, s_screen, s_passthrough };
	const enum textureIDs { t_frame, t_depth, t_dcalib };
	const enum vaoIDs	  { a_quad, a_button, a_projection };
	const enum vboIDs	  { v_quad, v_quad_tx, v_quad_ind, v_projection };

	//// Render state options ////
	std::string name;
	//transformation between camera and eye vantage point for see through AR
	glm::mat4 cam2eye = glm::mat4(1.f), projection = glm::mat4(1.f), depth_projection = glm::mat4(1.f), fixDepth = glm::mat4(1.f);
	//specifically for distortion correction
	glm::vec2 eyebox = glm::vec2(0.);
	float factor = 0.f;
	//depth buffer transformation
	std::vector<float> projection_verts;
	int pro_vert_size = -1;
	//viewport for screen
	glm::vec4 viewp = glm::vec4(0.f, 0.f, 0.f, 0.f);
	//aspect ratio and vertical fov
	float aspect = 0.f, aspect_d = 0.f, fov = 0.f, fov_dh = 0.f, fov_dv = 0.f;
	//mode, global time
	int mode = 0;
	//should be enough time ;)
	unsigned long long frameNumber = 0;

	//// Render features ////
	DistortionMesh dewarp;
	//let them override default shaders
	Shader *shaders[8];
	//this is all the renderer specific content
	GLuint vao[8], fbo[8], resID[8], textures[8], vbo[16];
	//handle for window
	GLFWwindow *window = nullptr;
	//handle for this context
	int ID = -1;

	//// Render state ////
	Context(i_Camera *cam, char *title);

	//sets the clear color
	void ClearColor(glm::vec4 &newCol);
	//shouldnt be called
	void Reshape(float _aspect, float _fov);

	//render content with unindexed geometry
	void RenderUnindexedContent(Content &cont, int fbo, int vao, std::vector<GLuint> textureUnits, std::vector<std::string> textureUniformNames, bool clear = true);
	//transform depth frame with shader
	void transformDepth();
	//show buffers on screen
	void display();
};

const int NUM_FBO = 4, NUM_TBO = 4, NUM_RT = 8, NUM_TEX = 16, NUM_VAO = 8, NUM_VBO = 16;

//The base container for openGL content
struct Content {
	//render context
	Context *target = nullptr;
	//shader to use
	Shader *shade = nullptr;
	//Rendering attributes and data
	const float *verts = nullptr, *norms = nullptr, *uvs = nullptr, *attr1 = nullptr, *attr2 = nullptr;
	const unsigned int *inds = nullptr, *attr3 = nullptr;
	//content model matrix
	glm::mat4 model = glm::mat4(1.f),
		//view matrix is from tracking usually
		view = glm::mat4(1.f),
		//inverse view transform for transforming camera instead of objects
		view_cam = glm::mat4(1.f),
		//normal matrix is transpose(inverse(MV))
		normalMatrix = glm::mat4(1.f);
	//opengl resources (frame buffers, transform buffers, render textures, textures, vertex arrays, vertex buffers)
	GLuint glFBO[4], glTBO[4], glRenTex[8], glTex[16], glVAO[8], glVBO[16];
	//yep
	int n_verts = 0, n_uvs = 0, n_inds = 0;
};
