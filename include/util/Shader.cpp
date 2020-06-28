#include <unordered_map>
#include <OpenGL/gl3.h>
#include "Log.h"
#include "Texture.h"
#include "Shader.h"


Shader::~Shader() {
	Log::debug("Deleting shader %d", mProgram);
	glDeleteProgram(mProgram);
	glDeleteShader(mVertexShader);
	glDeleteShader(mFragmentShader);
}


void Shader::link() const {
	glLinkProgram(mProgram);
	GLint isLinked = 0;
	glGetProgramiv(mProgram, GL_LINK_STATUS, (int*) &isLinked);
	if (isLinked == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(mProgram, GL_INFO_LOG_LENGTH, &length);

		//The maxLength includes the NULL character
		std::string log(length, ' ');
		glGetProgramInfoLog(mProgram, length, &length, &log[0]);
		Log::error("Shader link error: %s", log.c_str());

		this->~Shader();
	}
}


void Shader::bindAttribute(GLuint index, const GLchar *name) const {
	glBindAttribLocation(mProgram, index, name);
}


GLint Shader::getLocation(const std::string& param) {
	// if location is unknown, add it to the cache
	if (locations.find(param) == locations.end()) {
		GLint location = glGetUniformLocation(mProgram, param.c_str());
		locations[param] = location;
		if (location == -1)
			Log::info("Location not found for %s", param.c_str());
	}
	return locations[param];
}


void Shader::setUniform1i(int location, int number) const {
	glUniform1i(location, number);
}


void Shader::set(const std::string& param, const ITexture* texture) {
	GLint location = getLocation(param);
	if (location >= 0) {
		texture->bind();
		setUniform1i(location, texture->getSlot());
	}
}


void Shader::set(const std::string& param, const btVector3& vector) {
	GLint location = getLocation(param);
	if (location >= 0) {
		glUniform3f(location, vector.x(), vector.y(), vector.z());
	}
}


void Shader::set(const std::string& param, float value) {
	GLint location = getLocation(param);
	if (location >= 0) {
		glUniform1f(location, value);
	}
}


void Shader::set(const std::string& param, int value) {
	GLint location = getLocation(param);
	if (location >= 0) {
		glUniform1i(location, value);
	}
}


void Shader::set(const std::string& param, float v1, float v2) {
	GLint location = getLocation(param);
	if (location >= 0) {
		glUniform2f(location, v1, v2);
	}
}


void Shader::set(const std::string &param, const Matrix4x4 &matrix) {
	GLint location = getLocation(param);
	if (location >= 0) {
		glUniformMatrix4fv(location, 1 /*only setting 1 matrix*/, false /*transpose?*/, matrix.raw());
	}
}


void Shader::set4f(const std::string& param, float* value) {
	GLint location = getLocation(param);
	if (location >= 0) {
		glUniform4f(location, value[0], value[1], value[2], value[3]);
	}
}
