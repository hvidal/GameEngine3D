#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include "PhysicsBody.h"
#include "Log.h"


PhysicsBody::PhysicsBody(float mass, std::unique_ptr<btTriangleIndexVertexArray> vertexArray, std::unique_ptr<btMotionState> motionState, const btVector3& localInertia):
	mTriangleIndexVertexArray(std::move(vertexArray)),
	mMotionState(std::move(motionState))
{
	bool useQuantizedAabbCompression = true; // enable useQuantizedAabbCompression for better memory usage.
	mCollisionShape = std::make_unique<btBvhTriangleMeshShape>(mTriangleIndexVertexArray.get(), useQuantizedAabbCompression);

	btRigidBody::btRigidBodyConstructionInfo cInfo(
			mass,
			mMotionState.get(),
			mCollisionShape.get(),
			localInertia);

	mRigidBody = std::make_shared<btRigidBody>(cInfo);
}


PhysicsBody::PhysicsBody(float mass, std::unique_ptr<btCollisionShape> shape, std::unique_ptr<btMotionState> motionState, const btVector3 &localInertia):
	mCollisionShape(std::move(shape)),
	mMotionState(std::move(motionState))
{
	btRigidBody::btRigidBodyConstructionInfo cInfo(
			mass,
			mMotionState.get(),
			mCollisionShape.get(),
			localInertia);

	mRigidBody = std::make_shared<btRigidBody>(cInfo);
}


PhysicsBody::~PhysicsBody() {
	Log::debug("Deleting PhysicsBody");
}


void PhysicsBody::write(ISerializer* serializer) const {
	btTransform transform = mRigidBody->getWorldTransform();
	btMatrix3x3 m3x3 = transform.getBasis();
	serializer->write(transform.getOrigin());
	serializer->write(m3x3.getRow(0));
	serializer->write(m3x3.getRow(1));
	serializer->write(m3x3.getRow(2));
}


void PhysicsBody::read(ISerializer* serializer) const {
	btVector3 origin, row0, row1, row2;
	serializer->read(origin);
	serializer->read(row0);
	serializer->read(row1);
	serializer->read(row2);
	btTransform transform(btMatrix3x3(row0, row1, row2), origin);
	mRigidBody->setWorldTransform(transform);
}







