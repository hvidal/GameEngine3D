#pragma once

#include "BulletDynamics/Vehicle/btRaycastVehicle.h"
#include "btBulletDynamicsCommon.h"

#include "../SceneObject.h"
#include "../../util/ModelOBJRenderer.h"
#include "../../util/CollisionShapeFactory.h"
#include "../../util/GLShapeRenderer.h"
#include "../../util/math/Matrix4x4.h"
#include "Vehicle.h"
#include "KeyboardVehicleControl.h"


class FourWheels: public SceneObject, public Vehicle
{
	std::string mCarModelFilename;
	std::string mWheelModelFilename;
	btRaycastVehicle::btVehicleTuning mTuning;

	std::unique_ptr<btVehicleRaycaster> mVehicleRayCaster;
	std::unique_ptr<btRaycastVehicle> mVehicle;
	std::unique_ptr<btCollisionShape> mWheelShape;
	std::unique_ptr<GLShapeRenderer> mShapeRenderer;

	std::shared_ptr<ModelOBJRenderer> mCarModel;
	std::shared_ptr<ModelOBJRenderer> mWheelModel;

	float mWeight;
	float mEngineForce;
	float mBrakeForce;
	bool mRearGear;

	float mMaxEngineForce;
	float mMaxBrakeForce;
	float mVehicleSteering;
	float mSteeringIncrement; // todo: should depend on velocity
	float mSteeringClamp;
	float mWheelRadius;
	float mWheelWidth;
	float mWheelFriction;
	float mSuspensionStiffness;
	float mSuspensionDamping;
	float mSuspensionCompression;
	float mSuspensionRestLength;
	float mRollInfluence;

	btVector3 mInitialPosition;

	void buildVehicleShape(btTransform transform, float halfWidth, float halfHeight, float halfLength, float yShift, float zShift);
	void buildWheels(float halfWidth, float halfLength, float yShift, float zShift);
	void addWheel(const btVector3& connectionPointCS0, bool isFrontWheel);
	void render(bool opaque, const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) const;

public:
	static const std::string& SERIALIZE_ID;

	FourWheels(std::weak_ptr<btDynamicsWorld> dynamicsWorld, const btVector3& initialPosition, const std::string& carModelFilename, const std::string& wheelModelFilename);
	~FourWheels();

	void initPhysics(btTransform transform) override;

	void turnLeft() override;
	void turnRight() override;
	void moveForward() override;
	void brake() override;
	void resetSteering() noexcept override { mVehicleSteering = 0.f; }
	void resetEngineAndBrake() noexcept override { mEngineForce = 0.f; mBrakeForce = 0.f; }
	void jump() override;

	virtual void reset() override;
	virtual void updateSimulation() override;
	virtual void renderOpaque(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) override;
	virtual void renderTranslucent(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) override;
	virtual void render2d(IRenderer2d* renderer2d) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string, Factory> factory();
};

//-----------------------------------------------------------------------------
