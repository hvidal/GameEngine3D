#ifndef SPHERESHAPE_H_
#define SPHERESHAPE_H_

#include <string>
#include "btBulletDynamicsCommon.h"
#include "../SceneObject.h"
#include "../../util/ModelOBJRenderer.h"
#include "../../util/GLShapeRenderer.h"


class SphereShape: public SceneObject
{
private:
	const float mMass;
	const float mRadius;
	const std::string mFileName;

	std::shared_ptr<ModelOBJRenderer> mSphereModel;
	std::unique_ptr<GLShapeRenderer> mShapeRenderer;

public:
	static const std::string& SERIALIZE_ID;

	SphereShape(float mass, float radius, std::string filename, std::weak_ptr<btDynamicsWorld>&& dynamicsWorld) :
		SceneObject(std::move(dynamicsWorld)),
		mMass(mass),
		mRadius(radius),
		mFileName(filename)
	{
		mSphereModel = ModelOBJRendererCache::get(filename);
		mShapeRenderer = std::make_unique<GLShapeRenderer>();
	}

	virtual ~SphereShape() {
		Log::debug("Deleting SphereShape");
	}

	virtual void initPhysics(btTransform transform) override;
	virtual void updateSimulation() override;
	virtual void renderOpaque(const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};


#endif /* SPHERESHAPE_H_ */
