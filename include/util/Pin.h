#ifndef PIN_H
#define PIN_H

#include <LinearMath/btVector3.h>
#include <vector>
#include "Shader.h"


class Pin {
	GLuint mVbo;
	GLuint mNbo;
	GLuint mIbo;
	std::unique_ptr<IShader> mShader;
	std::unique_ptr<IShader> mShaderInstanced;
public:
	Pin();
	~Pin();

	void render(const btVector3& point, const ICamera* camera, const btVector3& lightPosition, const btVector3& color);
	void render(const std::vector<btVector3>& points, const ICamera* camera, const btVector3& lightPosition, const btVector3& color);
};

//-----------------------------------------------------------------------------

#endif
