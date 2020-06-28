#ifndef GAMEDEV3D_WATERREFLECTION_H
#define GAMEDEV3D_WATERREFLECTION_H

#include "../../app/Interfaces.h"
#include "../../util/FrameBuffer.h"


class SurfaceReflection: public ISurfaceReflection {

	float mPlanetRadius;
	float mWaterLevel;
	std::unique_ptr<ITexture> mColorTexture;
	std::unique_ptr<ITexture> mDepthTexture;
	std::unique_ptr<FrameBuffer> mBuffer;
	bool mIsRendering;

public:
	static const std::string& SERIALIZE_ID;

	SurfaceReflection(float planetRadius, float waterLevel, unsigned int textureWidth, unsigned int textureHeight);
	virtual ~SurfaceReflection();

	virtual const ITexture* getColorTexture() const override;
	virtual bool isRendering() const override;

	virtual void begin() override;
	virtual void end() override;

	virtual void setVars(IShader* shader, const ICamera* camera) const override;
	virtual void setReflectionTexture(IShader* shader) const override;

	static const char* getVertexShaderCode();

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

inline const ITexture* SurfaceReflection::getColorTexture() const
{ return mColorTexture.get(); }

inline bool SurfaceReflection::isRendering() const
{ return mIsRendering; }

#endif
