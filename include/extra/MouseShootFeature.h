#ifndef MOUSESHOOTFEATURE_H_
#define MOUSESHOOTFEATURE_H_

#include <forward_list>
#include "../app/Application.h"
#include "../camera/Camera.h"
#include "../scene/shape/SphereShape.h"
#include "../scene/planet/Planet.h"


class MouseShootFeature: public IMouseListener
{
private:
	std::weak_ptr<IGameScene> mGameScene;
	std::weak_ptr<Planet> mPlanet;
	std::forward_list<std::shared_ptr<ISceneObject>> mShapes;

	const IMouseListener::ButtonCode mActionButton;
	const std::string mFilename;

	void shoot(int x, int y);
public:
	MouseShootFeature(std::weak_ptr<IGameScene> gameScene, std::weak_ptr<Planet> planet, IMouseListener::ButtonCode actionButton, const std::string& filename):
		mGameScene(std::move(gameScene)),
		mPlanet(std::move(planet)),
		mActionButton(actionButton),
		mFilename(filename)
	{}

	~MouseShootFeature() {
		Log::debug("Deleting MouseShootFeature");
	}

	void mouseDown(Uint8 button, int x, int y, const GameState& gameState) override;
};

#endif