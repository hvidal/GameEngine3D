#ifndef PLANET_H
#define PLANET_H

#include <array>
#include "../../app/Interfaces.h"
#include "../SceneObject.h"
#include "PlanetFace.h"


class Planet: public SceneObject
{
	float mRadius;
	float mWaterLevel;
	float mBrushSize;

	std::array<std::unique_ptr<PlanetFace>, 6> mFaces;

	std::forward_list<std::shared_ptr<btRigidBody>> mInteractiveBodies;
	std::forward_list<std::shared_ptr<IPlanetExternalObject>> mExternalObjects;

	std::unique_ptr<IShader> mShader;
	std::unique_ptr<IShader> mWaterShader;

	std::array<std::shared_ptr<ITexture>,4> mTextureArray;

	bool mHasVisibleWater;
	std::unordered_map<unsigned int,float> mVisiblePages;

	void fixBorderNormals(const btVector3& mouse3d, float brushSize, const ICamera *camera, const btVector3& innerPoint);
	void runAction(const GameState& gameState, const ICamera* camera);
	void collectVisiblePageIds(const ICamera*);
protected:
	virtual ISceneObject::CommandMap getCommands() override;

	Planet(std::shared_ptr<btDynamicsWorld> dynamicsWorld, float radius, float waterLevel);
public:
	static const std::string& SERIALIZE_ID;

	Planet(std::shared_ptr<btDynamicsWorld> dynamicsWorld, float radius, float waterLevel, unsigned int pagesPerFace, unsigned int pageSize);
	virtual ~Planet();

	float getRadius() const noexcept;
	float getBrushSize() const noexcept;
	unsigned int getPageId(const btVector3&) const;
	btVector3 getSurfacePoint(btVector3 p, float extraIncrement = 0.f) const;
	btTransform getSurfaceTransform(btVector3 p, float extraIncrement = 0.f) const;
	void moveTo(Matrix4x4& matrix, const btVector3& direction, float speed);
	float getHeightAt(const btVector3& direction) const;
	static void convertPointToUV(const btVector3& point, float* uv);

	void setTexture(unsigned int index, std::unique_ptr<ITexture>&& colorTexture);
	virtual void initPhysics(btTransform transform) override;
	void interactWith(std::shared_ptr<btRigidBody> rigidBody) noexcept;
	void addExternalObject(std::shared_ptr<IPlanetExternalObject> externalObject);

	virtual void updateSimulation() override;
	virtual void renderOpaque(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) override;
	virtual void renderTranslucent(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

inline float Planet::getRadius() const noexcept
{ return mRadius; }

inline float Planet::getBrushSize() const noexcept
{ return mBrushSize; }

inline void Planet::interactWith(std::shared_ptr<btRigidBody> rigidBody) noexcept
{ mInteractiveBodies.push_front(rigidBody); }

inline void Planet::addExternalObject(std::shared_ptr<IPlanetExternalObject> externalObject)
{ mExternalObjects.push_front(externalObject); }

#endif

