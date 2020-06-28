#include "KeyboardCharacterControl.h"


KeyboardCharacterControl::KeyboardCharacterControl(std::shared_ptr<OBJCharacter> character, SDL_Keycode forwardKey, SDL_Keycode backwardKey, SDL_Keycode leftKey, SDL_Keycode rightKey):
	mCharacter(std::move(character)),
	mForwardKey(forwardKey),
	mBackwardKey(backwardKey),
	mLeftKey(leftKey),
	mRightKey(rightKey),
	currentSpeed(0.0f),
	mVehicleKey(SDLK_UNKNOWN),
	mVehicleMinDistance(5.0f),
	mCurrentVehicleIndex(-1)
{}


void KeyboardCharacterControl::addAction(unsigned int index, SDL_Keycode key, float speed) {
	mActions.push_back({ index, key, speed });
	// Store extra key
	mExtraKeys.insert(key);
};


void KeyboardCharacterControl::update(const ICamera* camera) {
	if (mCurrentVehicleIndex < 0) {
		bool isForwardDown = mDownKeys.count(mForwardKey) > 0;
		bool isBackwardDown = mDownKeys.count(mBackwardKey) > 0;
		bool isLeftDown = mDownKeys.count(mLeftKey) > 0;
		bool isRightDown = mDownKeys.count(mRightKey) > 0;

		if (isForwardDown || isBackwardDown || isLeftDown || isRightDown) {
			static btVector3 previousDirection(0.0f, 0.0f, 0.0f);
			btVector3 direction(0.0f, 0.0f, 0.0f);
			// Since we are looking from behind the camera,
			// the directions have to be reversed.
			if (isForwardDown)
				direction += -camera->getDirection();
			else if (isBackwardDown)
				direction += camera->getDirection();

			if (isLeftDown)
				direction += camera->getRight();
			else if (isRightDown)
				direction += -camera->getRight();

			direction = (direction.normalized() + 0.75f * previousDirection).normalized();
			mCharacter->moveTo(direction, currentSpeed);

			previousDirection = direction;
		}
	} else {
		// The vehicle is carrying the character, so we have to update the position of the character.
		const btVector3& vehiclePosition = mVehicles[mCurrentVehicleIndex]->getMatrix4x4().getOrigin();
		mCharacter->setPosition(vehiclePosition);
	}


}


void KeyboardCharacterControl::setCurrentAction(const Action& action) {
	currentSpeed = action.speed;
	mCharacter->setCurrentAnimation(action.index);
}


void KeyboardCharacterControl::selectAnimation() {
	bool hasMovement =
		mDownKeys.count(mForwardKey) > 0 ||
		mDownKeys.count(mBackwardKey) > 0 ||
		mDownKeys.count(mLeftKey) > 0 ||
		mDownKeys.count(mRightKey) > 0
	;

	if (hasMovement) {
		auto isExtraKeyDown = [this](){
			for (SDL_Keycode c : mExtraKeys) {
				if (mDownKeys.count(c) > 0)
					return true;
			}
			return false;
		};

		for (const auto& action : mActions) {
			if (action.speed == 0.0f)
				continue; // we only want movement actions

			if (action.key == SDLK_UNKNOWN) {
				// no extra key should be down
				if (!isExtraKeyDown()) {
					setCurrentAction(action);
					return;
				}
			} else {
				// Select this movement if its key is currently down
				if (mDownKeys.count(action.key) > 0) {
					setCurrentAction(action);
					return;
				}
			}
		}
	} else {
		// No movement, defaults to zero index
		setCurrentAction(mActions[0]);
	}
}


void KeyboardCharacterControl::keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) {
	mDownKeys.insert(key);

	if (key == mVehicleKey) {
		if (mCurrentVehicleIndex < 0) {
			// find vehicle close to character
			findVehicleNearby();
		} else {
			// leave vehicle
			leaveVehicle();
		}
	}

	// If not inside vehicle
	if (mCurrentVehicleIndex < 0)
		selectAnimation();
};


void KeyboardCharacterControl::keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) {
	mDownKeys.erase(key);
	selectAnimation();
}


void KeyboardCharacterControl::findVehicleNearby() {
	int i = 0;
	for (const auto& v : mVehicles) {
		float distance = mCharacter->getMatrix4x4().getOrigin().distance(v->getMatrix4x4().getOrigin());
		if (distance < mVehicleMinDistance) {
			mCurrentVehicleIndex = i;
			mCharacter->setVisible(false);
			mCharacter->setIsInVehicle(true);
			break;
		}
		i++;
	}
}


void KeyboardCharacterControl::leaveVehicle() {
	// Move the character to the side of the vehicle
	const Matrix4x4& vehicleMatrix4x4 = mVehicles[mCurrentVehicleIndex]->getMatrix4x4();
	const btVector3& toTheLeft = vehicleMatrix4x4.getRow(0);
	const btVector3& leftSide = vehicleMatrix4x4.getOrigin() + 2.0f * toTheLeft;
	mCharacter->setPosition(leftSide);
	mCharacter->setVisible(true);
	mCharacter->setIsInVehicle(false);

	mCurrentVehicleIndex = -1;
}
