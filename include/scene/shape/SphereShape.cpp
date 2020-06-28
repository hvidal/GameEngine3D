#include "SphereShape.h"
#include "../../app/Interfaces.h"


const std::string& SphereShape::SERIALIZE_ID = "SphereShape";


void SphereShape::initPhysics(btTransform transform)
{
	std::unique_ptr<btConvexInternalShape> shape = std::make_unique<btSphereShape>(mRadius);
	this->createPhysicsBody(mMass, transform, std::move(shape));
}


void SphereShape::updateSimulation() {
	const btVector3& centerOfMass = mPhysicsBody->getRigidBody()->getCenterOfMassPosition();
	const btVector3& gravity = -centerOfMass.normalized() * 25.f; // points to the origin (planet center)
	mPhysicsBody->getRigidBody()->setGravity(gravity);
}


void SphereShape::renderOpaque(const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState)
{
	btVector3 center = mPhysicsBody->getRigidBody()->getCenterOfMassPosition();
	if (!camera->isVisible(center))
		return;

	static Matrix4x4 m4x4;
	m4x4 = getMatrix4x4();

	if (gameState.debugCode == DebugCode::COLLISION_SHAPE)
		mShapeRenderer->render(camera, sky, mPhysicsBody->getCollisionShape(), m4x4, true);
	else
		mSphereModel->render(camera, sky, shadowMap, m4x4, true);
}


void SphereShape::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mMass);
	serializer->write(mRadius);
	serializer->write(mFileName);
	mPhysicsBody->write(serializer);
}


const std::string& SphereShape::serializeID() const noexcept {
	return SERIALIZE_ID;
}


std::pair<std::string, Factory> SphereShape::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		float mass, radius;
		std::string filename;
		serializer->read(mass);
		serializer->read(radius);
		serializer->read(filename);

		IGameScene* gameScene = window->getGameScene().get();

		std::shared_ptr<SphereShape> o = std::make_shared<SphereShape>(mass, radius, filename, gameScene->getDynamicsWorld());
		o->setObjectId(objectId);
		o->initPhysics(btTransform::getIdentity());
		o->mPhysicsBody->read(serializer);
		gameScene->addSceneObject(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}