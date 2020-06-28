#ifndef SCENEOBJECT_H_
#define SCENEOBJECT_H_

#include <unordered_map>
#include "btBulletDynamicsCommon.h"
#include "../app/Interfaces.h"
#include "../util/PhysicsBody.h"


class SceneObject: public ISceneObject
{
protected:
	float mCameraHeight {0.0f};
	float mMinCameraDistance {0.0f};
	float mMaxCameraDistance {0.0f};

	std::shared_ptr<PhysicsBody> mPhysicsBody;
	std::weak_ptr<btDynamicsWorld> mDynamicsWorld;

	void createPhysicsBody(float mass, const btTransform& startTransform, std::unique_ptr<btCollisionShape> shape);
public:
	SceneObject();
	SceneObject(std::weak_ptr<btDynamicsWorld>);
	virtual ~SceneObject();

	Matrix4x4 getMatrix4x4(btTransform& transform) const;
	virtual Matrix4x4 getMatrix4x4() const override;

	virtual float getCameraHeight() const noexcept override;
	void setCameraHeight(float height) noexcept;

	virtual float getMinCameraDistance() const noexcept override;
	virtual float getMaxCameraDistance() const noexcept override;
	void setCameraDistanceRange(float minDistance, float maxDistance) noexcept;

	std::shared_ptr<btRigidBody> getSharedRigidBody() const noexcept;
};

//-----------------------------------------------------------------------------

inline std::shared_ptr<btRigidBody> SceneObject::getSharedRigidBody() const noexcept
{ return mPhysicsBody->getSharedRigidBody(); }

inline float SceneObject::getCameraHeight() const noexcept
{ return mCameraHeight; }

inline float SceneObject::getMinCameraDistance() const noexcept
{ return mMinCameraDistance; }

inline float SceneObject::getMaxCameraDistance() const noexcept
{ return mMaxCameraDistance; }

inline void SceneObject::setCameraHeight(float height) noexcept
{ mCameraHeight = height; }

inline void SceneObject::setCameraDistanceRange(float minDistance, float maxDistance) noexcept
{ mMinCameraDistance = minDistance; mMaxCameraDistance = maxDistance; }

#endif
