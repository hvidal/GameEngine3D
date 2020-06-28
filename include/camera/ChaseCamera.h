#ifndef GAMEDEV3D_CHASECAMERA_H
#define GAMEDEV3D_CHASECAMERA_H

#include "Camera.h"
#include "../scene/planet/Planet.h"


class ChaseCamera: public Camera {

	std::shared_ptr<ISceneObject> mTargetObject;
	std::shared_ptr<Planet> mPlanet;

	const unsigned long mChaseObjectId;
public:
	static const std::string& SERIALIZE_ID;

	ChaseCamera(std::weak_ptr<IWindow> window, std::shared_ptr<ISceneObject> object, std::shared_ptr<Planet> planet) :
		Camera(std::move(window)),
		mPlanet(std::move(planet)),
		mTargetObject(object),
		mChaseObjectId(object->getObjectId())
	{}

	virtual ~ChaseCamera()
	{ Log::debug("Deleting ChaseCamera"); }

	void updateControls() override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

#endif //GAMEDEV3D_CHASECAMERA_H
