#include <SDL2/SDL_keycode.h>
#include <memory>
#include "KeyboardVehicleControl.h"


KeyboardVehicleControl::KeyboardVehicleControl(std::weak_ptr<Vehicle>&& vehicle, SDL_Keycode forwardKey, SDL_Keycode backwardKey, SDL_Keycode leftKey, SDL_Keycode rightKey, SDL_Keycode actionKey) :
	IKeyboardListener(),
	mVehicle(std::move(vehicle)),
	mForwardKey(forwardKey),
	mBackwardKey(backwardKey),
	mLeftKey(leftKey),
	mRightKey(rightKey),
	mActionKey(actionKey),
	forward(false),
	backward(false),
	left(false),
	right(false)
{}


void KeyboardVehicleControl::keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) {
	if (key == mLeftKey)
		left = true;
	else if (key == mRightKey)
		right = true;
	else if (key == mForwardKey)
		forward = true;
	else if (key == mBackwardKey)
		backward = true;
	else if (key == mActionKey)
		mVehicle.lock()->jump();
};


void KeyboardVehicleControl::keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) {
	if (key == mLeftKey)
		left = false;
	else if (key == mRightKey)
		right = false;
	else if (key == mForwardKey)
		forward = false;
	else if (key == mBackwardKey)
		backward = false;
};


void KeyboardVehicleControl::update(const ICamera* camera) {
	std::shared_ptr<Vehicle> vehicle = mVehicle.lock();
	if (forward)
		vehicle->moveForward();
	else if (backward)
		vehicle->brake();
	else
		vehicle->resetEngineAndBrake();

	if (left)
		vehicle->turnLeft();
	else if (right)
		vehicle->turnRight();
	else
		vehicle->resetSteering();
}