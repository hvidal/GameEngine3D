#include "OBJCharacter.h"


const std::string& OBJCharacter::SERIALIZE_ID = "OBJCharacter";

//-----------------------------------------------------------------------------

Matrix4x4 OBJCharacter::getMatrix4x4() const {
	return mMatrix;
}


void OBJCharacter::setPosition(const btVector3 &position) {
	mMatrix = mPlanet->getSurfaceTransform(position);
}


void OBJCharacter::addAnimation(std::unique_ptr<OBJAnimation>&& animation) {
	mAnimations.push_back(std::move(animation));
}


void OBJCharacter::setCurrentAnimation(unsigned int index) {
	if (currentAnimationIndex != index) {
		currentAnimationIndex = index;
		mAnimations[currentAnimationIndex]->start();
	}
}


void OBJCharacter::moveTo(const btVector3& direction, float speed) {
	if (speed > 0.0f) {
		mPlanet->moveTo(mMatrix, direction, speed);
	}
}


void OBJCharacter::renderOpaque(const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) {
	if (mIsVisible)
		mAnimations[currentAnimationIndex]->render(camera, sky, shadowMap, mMatrix, true);
}


void OBJCharacter::write(ISerializer *serializer) const {
	// Todo
}


std::pair<std::string, Factory> OBJCharacter::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		btTransform transform;
		std::shared_ptr<OBJCharacter> o = std::make_shared<OBJCharacter>(transform, nullptr);
		o->setObjectId(objectId);

		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string &OBJCharacter::serializeID() const noexcept {
	return SERIALIZE_ID;
}



