/*

Shader could be more user friendly
	-functionality for more stages like geometry and feedback
	-streamline adding and updating uniforms
	-perhaps comb through the shader program and extract uniforms
	-include pointer to data and upload properly

*/

#pragma once

#define DBG_SHADER

#include "winderps.h"

#include <unordered_map>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

class Shader {
	GLuint id = NULL;
	//use dictionary or something
	std::unordered_map<std::string, GLuint> uniforms;

public:
	
	Shader() {}
	//creates and loads a shader
	Shader(std::string vertex_file_path, std::string fragment_file_path, bool feedback = false);
	//creates and loads a shader
	Shader(std::string geometry_file_path, std::string vertex_file_path, std::string fragment_file_path, bool feedback = false);

	//loads a .vert .frag pair
	bool loadShader(std::string vertex_file_path, std::string fragment_file_path, bool feedback = false);
	//loads a .geo .vert .frag series
	bool loadShader(std::string geometry_file_path, std::string vertex_file_path, std::string fragment_file_path, bool feedback = false);

	//getters
	GLuint getID() { return id; }
	GLuint uniform(std::string name) { return uniforms[name]; }

	//adds a uniform to the pipeline
	void addUniform(std::string name);
	//add a few uniforms
	void addUniforms(std::vector<std::string> n);
};
