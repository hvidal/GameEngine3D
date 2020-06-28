#ifndef PLANETFACE_H
#define PLANETFACE_H

#include <string>
#include <forward_list>
#include "PlanetPage.h"
#include "IPlanetExternalObject.h"


class PlanetFace
{
	float mVisibleLimit;
	std::vector<std::unique_ptr<PlanetPage>> mPages;

	std::unique_ptr<FieldOfView> mFOD;
	btVector3 mCorners[4];

	mutable std::pair<unsigned long, bool> mCameraVisibility {-1, false};
	bool isVisible(const ICamera* camera) const;
	void setFieldOfView();

	PlanetFace();
public:
	PlanetFace(unsigned int faceId, const std::string& plane, float radius, float waterLevel, unsigned int faceDivisions, unsigned int pageDivisions);

	void enablePhysics(btDynamicsWorld *dynamicsWorld, const std::forward_list<std::shared_ptr<btRigidBody>>& rigidBodies);
	float getHeightAt(const btVector3& direction) const;
	void getNormalAt(const btVector3& point, btVector3& normal) const;
	void editVertices(const std::string& command, float value, const btVector3& point3D, float brushSize, const ICamera* camera, const btVector3& innerPoint);
	void fixBorderNormals(const std::function<void(const btVector3&,btVector3&)>& getNormalAt, const btVector3& mouse3d, float brushSize, const ICamera* camera, const btVector3& innerPoint);
	void autoPaintVertices(const std::string& param, const btVector3& point3D, float brushSize, const ICamera* camera, const btVector3& innerPoint);
	unsigned int getPageId(const btVector3&) const;
	size_t getPageCount() const noexcept;
	void getVisiblePageIds(const ICamera*, const btVector3& innerPoint, std::unordered_map<unsigned int,float>&, bool& hasVisibleWater) const;

	void initPhysics(btDynamicsWorld* dynamicsWorld);
	void renderOpaque(const std::unordered_map<unsigned int,float>& visiblePages, const ICamera* camera, const GameState& gameState);
	void renderTranslucent(const std::unordered_map<unsigned int,float>& visiblePages, const ICamera* camera, const GameState& gameState);

	void write(ISerializer *serializer) const;
	static std::string serializeID();
	static std::unique_ptr<PlanetFace> create(ISerializer*, std::weak_ptr<btDynamicsWorld>);
};

//-----------------------------------------------------------------------------

inline size_t PlanetFace::getPageCount() const noexcept
{ return mPages.size(); }

#endif