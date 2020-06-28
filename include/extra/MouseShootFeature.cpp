#include "MouseShootFeature.h"


void MouseShootFeature::mouseDown(Uint8 button, int x, int y, const GameState& gameState)
{
	// Not perfect, but SDL provides the button as Uint8, not as an enum class.
	if (button == static_cast<int>(mActionButton) && gameState.mode == GameMode::RUNNING)
		shoot(x, y);
}


void MouseShootFeature::shoot(int x, int y)
{
	Log::debug("Shooting %d, %d", x, y);

	std::shared_ptr<IGameScene> psGameScene = mGameScene.lock();
	const ICamera* pCamera = psGameScene->getActiveCamera();

	btTransform transform;
	transform.setIdentity();
	const btVector3& camPos = pCamera->getPosition();
	transform.setOrigin(camPos);

	std::shared_ptr<SphereShape> shape = std::make_shared<SphereShape>(8.f, 1.5f, mFilename, psGameScene->getDynamicsWorld());
	mShapes.push_front(shape);
	shape->initPhysics(transform);
	psGameScene->addSceneObject(shape);

	std::shared_ptr<btRigidBody> body = shape->getSharedRigidBody();
	body->setLinearFactor(btVector3(1.f,1.f,1.f));
	body->setRestitution(1);

	const btVector3 rayTo = pCamera->getRayTo(x, y);
	btVector3 linVel = rayTo - camPos;
	linVel.normalize();
	linVel *= 50.f; // shoot speed

	body->getWorldTransform().setOrigin(camPos);
	body->getWorldTransform().setRotation(btQuaternion(0,0,0,1));
	body->setLinearVelocity(linVel);
	body->setAngularVelocity(btVector3(0,0,0));
	body->setCcdMotionThreshold(0.5);
	body->setCcdSweptSphereRadius(0.9f);

	mPlanet.lock()->interactWith(body);
}