#ifndef SHADER_H_
#define SHADER_H_

#include <unordered_map>
#include <OpenGL/gl3.h>
#include <LinearMath/btVector3.h>
#include "../app/Interfaces.h"
#include "Log.h"

class Shader: public IShader
{
	GLuint mVertexShader;
	GLuint mFragmentShader;
	GLuint mProgram;

	std::unordered_map<std::string, GLint> locations;

	template <int N> GLuint compile(GLuint type, const char* const(&source)[N]) const;
public:
	template <int N, int M> Shader(const char* const(&vs)[N], const char* const(&fs)[M]);
	virtual ~Shader();

	virtual void run() const override;
	virtual void link() const override;

	GLint getLocation(const std::string& param);
	virtual void bindAttribute(GLuint index, const GLchar *name) const override;
	void setUniform1i(int location, int number) const;
	virtual void set(const std::string& param, const ITexture* texture) override;
	virtual void set(const std::string& param, const btVector3& vector) override;
	virtual void set(const std::string& param, float value) override;
	virtual void set(const std::string& param, int value) override;
	virtual void set(const std::string& param, float v1, float v2) override;
	virtual void set(const std::string& param, const Matrix4x4& matrix) override;
	virtual void set4f(const std::string& param, float* value) override;
};

//-----------------------------------------------------------------------------

inline void Shader::run() const
{ glUseProgram(mProgram); }


template <int N> GLuint Shader::compile(GLuint type, const char* const(&source)[N]) const {
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, N, source, NULL);
	glCompileShader(shader);
	GLint compiled;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		std::string log(length, ' ');
		glGetShaderInfoLog(shader, length, &length, &log[0]);
		std::string msg { "Shader not compiled: " + log };
		Log::error(msg.c_str());
		glDeleteShader(shader); // Don't leak the shader.

		for (int i = 0; i < N; i++) {
			Log::debug(source[i]);
		}

		exit(-1);
	}
	return shader;
}


template <int N, int M> Shader::Shader(const char* const(&vs)[N], const char* const(&fs)[M]) {
	mVertexShader = compile(GL_VERTEX_SHADER, vs);
	mFragmentShader = compile(GL_FRAGMENT_SHADER, fs);
	mProgram = glCreateProgram();
	glAttachShader(mProgram, mVertexShader);
	glAttachShader(mProgram, mFragmentShader);
	glLinkProgram(mProgram);
}


#endif