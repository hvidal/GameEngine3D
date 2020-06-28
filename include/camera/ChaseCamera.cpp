#include "ChaseCamera.h"

const std::string& ChaseCamera::SERIALIZE_ID = "ChaseCamera";

//-----------------------------------------------------------------------------

void ChaseCamera::updateControls() {
	// look at the object
	mTargetPosition = mTargetObject->getMatrix4x4().getOrigin();
	mUp = mTargetPosition.normalized();

	const float cameraHeight = mTargetObject->getCameraHeight();
	const float minCameraDistance = mTargetObject->getMinCameraDistance();
	const float maxCameraDistance = mTargetObject->getMaxCameraDistance();

	// interpolate the camera height
	// we give priority to the current height so that the camera won't jump too much
	float height = (10.0f * mPosition.length() + mTargetPosition.length() + cameraHeight) / 11.0f;
	mPosition = height * mPosition.normalized();

	const btVector3&& vCameraToTarget = getDirection();
	// keep distance between min and max distance
	float cameraDistance = vCameraToTarget.length();
	float beyondRange =
			cameraDistance < minCameraDistance? minCameraDistance :
			cameraDistance > maxCameraDistance? maxCameraDistance : 0.0f;

	if (beyondRange > 0.0f) {
		float correctionFactor = 0.15f * (beyondRange - cameraDistance) / cameraDistance;
		mPosition -= correctionFactor * vCameraToTarget;
	}

	// make sure the camera is not under the terrain
	float minHeight = mPlanet->getHeightAt(mPosition) + 4.0f;
	if (mPosition.length() < minHeight) {
		mPosition = minHeight * mPosition.normalized();
	}

	mVersion++;
}


void ChaseCamera::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mChaseObjectId);
	writeVars(serializer);
}


std::pair<std::string, Factory> ChaseCamera::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(window->getGameScene()->getSerializable(Planet::SERIALIZE_ID));

		unsigned long chaseObjectId;
		serializer->read(chaseObjectId);
		std::shared_ptr<SceneObject> object = std::dynamic_pointer_cast<SceneObject>(window->getGameScene()->getByObjectId(chaseObjectId));

		std::shared_ptr<ChaseCamera> o = std::make_shared<ChaseCamera>(window, object, planet);
		o->setObjectId(objectId);
		o->readVars(serializer);

		IGameScene* pGameScene = window->getGameScene().get();
		pGameScene->setGameCamera(o);
		pGameScene->addSerializable(o);
		window->addResizeListener([o](int w, int h) { o->windowResized(); });
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& ChaseCamera::serializeID() const noexcept {
	return SERIALIZE_ID;
}











