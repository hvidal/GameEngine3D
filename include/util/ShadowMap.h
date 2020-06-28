#ifndef SHADOWMAP_H_
#define SHADOWMAP_H_

#include "../app/Interfaces.h"
#include "FrameBuffer.h"


class ShadowMap: public IShadowMap {
	std::unique_ptr<ITexture> mDepthTexture;
	std::unique_ptr<FrameBuffer> mFBO;
	bool mIsRendering;

	Matrix4x4 mViewMatrix;
	Matrix4x4 mProjectionMatrix;
	Matrix4x4 mShadowMVP;

	void setTextureMatrix();

public:
	static const std::string& SERIALIZE_ID;

	ShadowMap(GLuint slot, unsigned int size);
	virtual ~ShadowMap();

	virtual void start(const btVector3& lightPosition, const btVector3& targetPosition, const btVector3& up) override;
	virtual void end() override;
	virtual bool isRendering() const noexcept override;

	virtual void setVars(IShader* shader) const override;
	virtual void setMatrices(IShader* shader) const override;

	static constexpr const char* getVertexShaderCode() noexcept;
	static constexpr const char* getFragmentShaderCode() noexcept;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

inline bool ShadowMap::isRendering() const noexcept
{ return mIsRendering; }

//-----------------------------------------------------------------------------

static constexpr const char* vertexShaderCode =
	"uniform mat4 shadowMVP;"
	"varying vec4 shadowCoord;"
	"void setShadowMap(vec4 p) {"
		"shadowCoord = shadowMVP * p;"
	"}"
	"void setShadowMap(vec3 p) {"
		"setShadowMap(vec4(p, 1.0));"
	"}";

static constexpr const char* fragmentShaderCode =
	"uniform sampler2D shadowMap;"
	"uniform float shadowSize;"
	"varying vec4 shadowCoord;"

	"float lookup(vec3 v, float dx, float dy, float offset) {"
		"vec4 uv = shadowCoord + vec4(dx * offset, dy * offset, 0.0, 0.0);"
		"float depth = texture2D(shadowMap, uv.xy).z;"
		"float distance = shadowCoord.z;"
		"return depth < distance? 0.0 : 1.0;"
	"}"

	"float getShadow9(vec3 v) {"
		// If not visible in the shadow map, then return;
		// http://stackoverflow.com/questions/6301085/how-to-check-if-an-object-lies-outside-the-clipping-volume-in-opengl
		// (ranges for shadowMap are different)
		"bool rangeX = shadowCoord.x > 0.0 && shadowCoord.x < shadowCoord.w;"
		"bool rangeY = shadowCoord.y > 0.0 && shadowCoord.y < shadowCoord.w;"
		"bool rangeZ = shadowCoord.z > 0.0 && shadowCoord.z < shadowCoord.w;"
		"bool inFrustum = rangeX && rangeY && rangeZ;"
		"if (!inFrustum) return 1.0;"

		"float offset = shadowCoord.w / shadowSize;"
		"float d = 1.5;"

		"float v1 = lookup(v, 0.0, 0.0, offset);"
		"float v2 = lookup(v, d, 0.0, offset);"
		"float v3 = lookup(v, -d, 0.0, offset);"
		"float v4 = lookup(v, 0.0, d, offset);"
		"float v5 = lookup(v, 0.0, -d, offset);"
		"float v6 = lookup(v, -d, -d, offset);"
		"float v7 = lookup(v, d, -d, offset);"
		"float v8 = lookup(v, -d, d, offset);"
		"float v9 = lookup(v, d, d, offset);"
		"float s = (v1+v2+v3+v4+v5+v6+v7+v8+v9) / 9.0;"
		"s = clamp(s, 0.0, 1.0);"
		"return s;"
	"}"
;

inline constexpr const char* ShadowMap::getVertexShaderCode() noexcept
{ return vertexShaderCode; }

inline constexpr const char* ShadowMap::getFragmentShaderCode() noexcept
{ return fragmentShaderCode; }

#endif