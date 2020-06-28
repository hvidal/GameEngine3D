#ifndef GAMEDEV3D_KEYBOARDCHARACTERCONTROL_H
#define GAMEDEV3D_KEYBOARDCHARACTERCONTROL_H

#include "../../app/Interfaces.h"
#include "OBJCharacter.h"


class KeyboardCharacterControl: public IKeyboardListener {
	std::shared_ptr<OBJCharacter> mCharacter;
	std::shared_ptr<Planet> mPlanet;
	SDL_Keycode mForwardKey, mBackwardKey, mLeftKey, mRightKey;

	struct Action {
		unsigned int index;
		SDL_Keycode key;
		float speed;
	};

	float currentSpeed;
	std::vector<Action> mActions;
	std::set<SDL_Keycode> mExtraKeys;
	std::set<SDL_Keycode> mDownKeys;

	// Vehicles
	SDL_Keycode mVehicleKey;	// key to enter/leave vehicle
	float mVehicleMinDistance;	// min distance to enter a vehicle
	int mCurrentVehicleIndex; 	// current vehicle used by the character
	std::vector<std::shared_ptr<ISceneObject>> mVehicles;

	void findVehicleNearby();
	void leaveVehicle();

	void selectAnimation();
	void setCurrentAction(const Action& index);
public:
	KeyboardCharacterControl(std::shared_ptr<OBJCharacter> character, SDL_Keycode forwardKey, SDL_Keycode backwardKey, SDL_Keycode leftKey, SDL_Keycode rightKey);

	void addAction(unsigned int index, SDL_Keycode key, float speed);

	// Vehicles
	void enableVehicles(SDL_Keycode key) noexcept;
	void addVehicle(std::shared_ptr<ISceneObject> vehicle) noexcept;

	void keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	void keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	void update(const ICamera* camera) override;
};

//-----------------------------------------------------------------------------

inline void KeyboardCharacterControl::enableVehicles(SDL_Keycode key) noexcept
{ mVehicleKey = key; }

inline void KeyboardCharacterControl::addVehicle(std::shared_ptr<ISceneObject> vehicle) noexcept
{ mVehicles.push_back(vehicle); }

#endif
