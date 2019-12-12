#include "shader.h"

//tell openGL to do all the work for us
bool compileShader(GLuint shaderID, std::string shaderCode) {
	GLint Result = GL_FALSE;
	int InfoLogLength;
	// Compile Geometry Shader
	glShaderSource(shaderID, 1, (const char* const*)shaderCode.c_str(), NULL);
	glCompileShader(shaderID);

	// Check Geometry Shader
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0){
		std::vector<char> shaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(shaderID, InfoLogLength, NULL, &shaderErrorMessage[0]);
#ifdef DBG_SHADER
		printf("Compilation error:\n%s", &shaderErrorMessage[0]);
#endif
		return false;
	}
	return true;
}

//creates and loads a shader
Shader::Shader(std::string vertex_file_path, std::string fragment_file_path, bool feedback) {
	loadShader(vertex_file_path, fragment_file_path, feedback);
}
//creates and loads a shader
Shader::Shader(std::string geometry_file_path, std::string vertex_file_path, std::string fragment_file_path, bool feedback) {
	loadShader(geometry_file_path, vertex_file_path, fragment_file_path, feedback);
}
//adds a uniform to the pipeline
void Shader::addUniform(std::string name) {
	uniforms[name] = glGetUniformLocation(id, name.c_str());
}
//adds a few uniforms
void Shader::addUniforms(std::vector<std::string> n) {
	for (int i = 0; i < n.size(); i++) uniforms[n[i]] = glGetUniformLocation(id, n[i].c_str());
}
//loads a .vert .frag program
bool Shader::loadShader(std::string vertex_file_path, std::string fragment_file_path, bool feedback) {
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	if (!openTxt(vertex_file_path, VertexShaderCode)) return false;

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	if (!openTxt(fragment_file_path, FragmentShaderCode)) return false;

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	char const *VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
#ifdef DBG_SHADER
		printf("%s\tERROR:\n%s\n", vertex_file_path.c_str(), &VertexShaderErrorMessage[0]);
#endif
		return false;
	}

	// Compile Fragment Shader
	char const *FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1){
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
#ifdef DBG_SHADER
		printf("%s\tERROR:\n%s\n", fragment_file_path.c_str(), &FragmentShaderErrorMessage[0]);
#endif
		return false;
	}

	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);

	if (feedback) {
		const GLchar *feedbackVaryings[] = { "feedback" };
		glTransformFeedbackVaryings(ProgramID, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	}

	glLinkProgram(ProgramID);

	// Check the program again?
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 1) {
#ifdef DBG_SHADER
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s / %s: (%i)\n%s\n", vertex_file_path.c_str(), fragment_file_path.c_str(), int(ProgramErrorMessage.size()), &ProgramErrorMessage[0]);
#endif
		return false;
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	id = ProgramID;

	return true;
}
//loads a .geo .vert .frag program
bool Shader::loadShader(std::string geometry_file_path, std::string vertex_file_path, std::string fragment_file_path, bool feedback) {
	bool win = true;
	//load the shaders
	std::string GeometryShaderCode, VertexShaderCode, FragmentShaderCode;
	win &= openTxt(geometry_file_path, GeometryShaderCode);
	win &= openTxt(vertex_file_path, VertexShaderCode);
	win &= openTxt(fragment_file_path, FragmentShaderCode);

	if (!win) {
#ifdef DBG_SHADER
		printf("Could not load shaders:\n\t%s\n\t%s\n\t%s", geometry_file_path.c_str(), vertex_file_path.c_str(), fragment_file_path.c_str());
#endif
		return false;
	}

	//create the shaders
	GLuint GeometryShaderID = glCreateShader(GL_GEOMETRY_SHADER);
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
	
	//compile the shaders
	win &= compileShader(GeometryShaderID, GeometryShaderCode);
	win &= compileShader(VertexShaderID, VertexShaderCode);
	win &= compileShader(FragmentShaderID, FragmentShaderCode);

	if (!win) return false;

	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, GeometryShaderID);
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);

	if (feedback) {
		const GLchar *feedbackVaryings[] = { "feedback" };
		glTransformFeedbackVaryings(ProgramID, 1, feedbackVaryings, GL_INTERLEAVED_ATTRIBS);
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	glLinkProgram(ProgramID);
	//check program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0){
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);

#ifdef DBG_SHADER
		printf("Program error:\n%s\n", &ProgramErrorMessage[0]);
#endif
		win = false;
	}
	//clean up
	glDeleteShader(GeometryShaderID);
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	id = ProgramID;

	return true;
}
