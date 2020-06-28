#ifndef INCLUDE_UTIL_OBJCHARACTER_H_
#define INCLUDE_UTIL_OBJCHARACTER_H_

#include <set>
#include "../../util/OBJAnimation.h"
#include "../SceneObject.h"
#include "../planet/Planet.h"


class OBJCharacter: public SceneObject {

	bool mIsVisible;
	Matrix4x4 mMatrix;
	std::shared_ptr<Planet> mPlanet;
	unsigned int currentAnimationIndex;

	std::vector<std::unique_ptr<OBJAnimation>> mAnimations;

	// Vehicles
	bool mIsInVehicle;
	float mVehicleCameraHeight;
	float mVehicleMinCameraDistance;
	float mVehicleMaxCameraDistance;

public:
	static const std::string& SERIALIZE_ID;

	OBJCharacter(const btTransform& transform, std::shared_ptr<Planet> planet):
		mIsVisible(true),
		mIsInVehicle(false),
		mPlanet(planet),
		mMatrix(transform),
		currentAnimationIndex(0)
	{}

	void setVisible(bool visible) noexcept;
	virtual Matrix4x4 getMatrix4x4() const override;
	void setPosition(const btVector3& position);

	void addAnimation(std::unique_ptr<OBJAnimation>&& animation);
	void setCurrentAnimation(unsigned int index);
	void moveTo(const btVector3& direction, float speed);

	void setIsInVehicle(bool isInVehicle) noexcept;
	void setVehicleCameraHeight(float height) noexcept;
	void setVehicleCameraDistanceRange(float minDistance, float maxDistance) noexcept;

	virtual float getCameraHeight() const noexcept override;
	virtual float getMinCameraDistance() const noexcept override;
	virtual float getMaxCameraDistance() const noexcept override;

	void renderOpaque(const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

inline void OBJCharacter::setVisible(bool visible) noexcept
{ mIsVisible = visible; }

inline void OBJCharacter::setIsInVehicle(bool isInVehicle) noexcept
{ mIsInVehicle = isInVehicle; }

inline float OBJCharacter::getCameraHeight() const noexcept
{ return mIsInVehicle? mVehicleCameraHeight : mCameraHeight; }

inline float OBJCharacter::getMinCameraDistance() const noexcept
{ return mIsInVehicle? mVehicleMinCameraDistance : mMinCameraDistance; }

inline float OBJCharacter::getMaxCameraDistance() const noexcept
{ return mIsInVehicle? mVehicleMaxCameraDistance : mMaxCameraDistance; }

inline void OBJCharacter::setVehicleCameraHeight(float height) noexcept
{ mVehicleCameraHeight = height; }

inline void OBJCharacter::setVehicleCameraDistanceRange(float minDistance, float maxDistance) noexcept
{ mVehicleMinCameraDistance = minDistance; mVehicleMaxCameraDistance = maxDistance; }

#endif
