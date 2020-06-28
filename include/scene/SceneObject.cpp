#include "SceneObject.h"


SceneObject::SceneObject()
{}


SceneObject::SceneObject(std::weak_ptr<btDynamicsWorld> dynamicsWorld):
	mDynamicsWorld(dynamicsWorld)
{}


SceneObject::~SceneObject()
{}


Matrix4x4 SceneObject::getMatrix4x4() const
{
	static float m16[16];
	btDefaultMotionState* myMotionState = dynamic_cast<btDefaultMotionState*>(mPhysicsBody->getRigidBody()->getMotionState());
	if (myMotionState)
		myMotionState->m_graphicsWorldTrans.getOpenGLMatrix(m16);
	else
		mPhysicsBody->getRigidBody()->getWorldTransform().getOpenGLMatrix(m16);

	return Matrix4x4(m16);
}


Matrix4x4 SceneObject::getMatrix4x4(btTransform& transform) const {
	static float m16[16];
	transform.getOpenGLMatrix(m16);
	return Matrix4x4(m16);
}


void SceneObject::createPhysicsBody(float mass, const btTransform& startTransform, std::unique_ptr<btCollisionShape> shape)
{
	static btVector3 localInertia(0,0,0);
	// btRigidBody is dynamic if and only if mass is non zero, otherwise static
	if (mass != 0.f)
		shape->calculateLocalInertia(mass, localInertia);

	std::unique_ptr<btMotionState> puMotionState = std::make_unique<btDefaultMotionState>(startTransform);

	mPhysicsBody = std::make_unique<PhysicsBody>(mass, std::move(shape), std::move(puMotionState), localInertia);
	mPhysicsBody->getCollisionShape()->setMargin(.2f);

	btScalar defaultContactProcessingThreshold(BT_LARGE_FLOAT);
	mPhysicsBody->getRigidBody()->setContactProcessingThreshold(defaultContactProcessingThreshold);

	mDynamicsWorld.lock()->addRigidBody(mPhysicsBody->getRigidBody());
}





