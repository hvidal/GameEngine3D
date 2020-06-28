#ifndef GAMEDEV3D_PHYSICS_BODY_H
#define GAMEDEV3D_PHYSICS_BODY_H

#include <memory>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LinearMath/btDefaultMotionState.h>
#include <BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h>
#include <forward_list>
#include "../app/Interfaces.h"

/**
 * Utility class that wraps a btRigidBody and related objects.
 */

class PhysicsBody {
	std::shared_ptr<btRigidBody> mRigidBody;
	std::unique_ptr<btCollisionShape> mCollisionShape;
	std::unique_ptr<btMotionState> mMotionState;
	std::unique_ptr<btTriangleIndexVertexArray> mTriangleIndexVertexArray;
	std::forward_list<std::unique_ptr<btCollisionShape>> mChildShapes;
public:
	PhysicsBody(float mass, std::unique_ptr<btTriangleIndexVertexArray> vertexArray, std::unique_ptr<btMotionState> motionState, const btVector3& localInertia);
	PhysicsBody(float mass, std::unique_ptr<btCollisionShape> shape, std::unique_ptr<btMotionState> motionState, const btVector3& localInertia);
	~PhysicsBody();

	void addChildShape(std::unique_ptr<btCollisionShape>);

	std::shared_ptr<btRigidBody> getSharedRigidBody() const noexcept;
	btRigidBody* getRigidBody() const noexcept;
	btCollisionShape* getCollisionShape() const noexcept;
	btMotionState* getMotionState() const noexcept;

	void write(ISerializer*) const;
	void read(ISerializer*) const;
};

//-----------------------------------------------------------------------------

inline std::shared_ptr<btRigidBody> PhysicsBody::getSharedRigidBody() const noexcept
{ return mRigidBody; }

inline btRigidBody* PhysicsBody::getRigidBody() const noexcept
{ return mRigidBody.get(); }

inline btCollisionShape* PhysicsBody::getCollisionShape() const noexcept
{ return mCollisionShape.get(); }

inline btMotionState* PhysicsBody::getMotionState() const noexcept
{ return mMotionState.get(); }

inline void PhysicsBody::addChildShape(std::unique_ptr<btCollisionShape> shape)
{ mChildShapes.push_front(std::move(shape)); }

#endif
