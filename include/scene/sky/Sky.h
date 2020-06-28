#ifndef GAMEDEV3D_SKY_H
#define GAMEDEV3D_SKY_H

#include "../SceneObject.h"


class Sky: public ISky, public SceneObject {

	float mAmbient;
	float mDiffuse;
	float mSpecular;

	// Sun position
	btVector3 mSunPosition;

	static constexpr float fixRange(float v) { return v < -1.f? -1.f : v > 1.f? 1.f : v; }

	float mPlanetRadius;
	float mWaterLevel;
	std::unique_ptr<IShader> mShader;
	GLuint mIbo;
	GLuint mVbo;

	void bind();
public:
	static const std::string& SERIALIZE_ID;

	Sky(float, float, const btVector3&);
	virtual ~Sky();

	virtual float getAmbientLight() const noexcept override;
	virtual void setAmbientLight(float ambient) noexcept override;

	virtual float getDiffuseLight() const noexcept override;
	virtual void setDiffuseLight(float diffuse) noexcept override;

	virtual float getSpecularLight() const noexcept override;
	virtual void setSpecularLight(float specular) noexcept override;

	virtual void setSunPosition(btVector3 sunPos) noexcept override;
	virtual const btVector3& getSunPosition() const noexcept override;

	virtual void setVars(IShader* shader, const ICamera* camera) const override;

	virtual void debug() const override;

	virtual void renderOpaque(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

inline float Sky::getAmbientLight() const noexcept
{ return mAmbient; }

inline void Sky::setAmbientLight(float ambient) noexcept
{ mAmbient = fixRange(ambient); }

inline float Sky::getDiffuseLight() const noexcept
{ return mDiffuse; }

inline void Sky::setDiffuseLight(float diffuse) noexcept
{ mDiffuse = fixRange(diffuse); }

inline float Sky::getSpecularLight() const noexcept
{ return mSpecular; }

inline void Sky::setSpecularLight(float specular) noexcept
{ mSpecular = fixRange(specular); }

inline void Sky::setSunPosition(btVector3 sunPos) noexcept
{ mSunPosition = sunPos; }

inline const btVector3& Sky::getSunPosition() const noexcept
{ return mSunPosition; }

#endif
