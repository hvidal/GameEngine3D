#pragma once


#include "../../app/Interfaces.h"
#include "Vehicle.h"


class KeyboardVehicleControl: public IKeyboardListener
{
private:
	std::weak_ptr<Vehicle> mVehicle;
	SDL_Keycode mForwardKey, mBackwardKey, mLeftKey, mRightKey, mActionKey;
	bool forward, backward, left, right;

public:
	KeyboardVehicleControl(std::weak_ptr<Vehicle>&& vehicle, SDL_Keycode forwardKey, SDL_Keycode backwardKey, SDL_Keycode leftKey, SDL_Keycode rightKey, SDL_Keycode actionKey);

	void keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	void keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	void update(const ICamera* camera) override;
};

