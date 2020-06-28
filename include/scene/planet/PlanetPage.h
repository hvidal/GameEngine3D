#ifndef PLANETPAGE_H
#define PLANETPAGE_H

#include <string>
#include <vector>
#include <forward_list>
#include <LinearMath/btTransform.h>
#include <BulletDynamics/Dynamics/btDynamicsWorld.h>
#include <BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h>
#include "../../app/Interfaces.h"
#include "IPlanetExternalObject.h"
#include "../../util/math/FieldOfView.h"
#include "../../util/PhysicsBody.h"


class PlanetPage {

	const unsigned int mPageId;
	float mPlanetRadius;
	float mWaterLevel;
	bool mHasWater;
	unsigned int mTriangleCount;
	unsigned int mVerticeCount;
	bool mIsActive;

	std::unique_ptr<FieldOfView> mFOD;

	// Stores the last camera version and isVisible
	mutable std::pair<unsigned long, bool> mCameraVisibility {-1, false};

	struct PlanetPageVertex {
		btVector3 position;
		btVector3 normal;
		float material[4];
		float uv[2]; // uv

		void write(ISerializer* serializer) const {
			serializer->write(position);
			serializer->write(normal);
			serializer->write(material[0]);
			serializer->write(material[1]);
			serializer->write(material[2]);
			serializer->write(material[3]);
			serializer->write(uv[0]);
			serializer->write(uv[1]);
		}

		static PlanetPageVertex read(ISerializer* serializer) {
			PlanetPageVertex v;
			serializer->read(v.position);
			serializer->read(v.normal);
			serializer->read(v.material[0]);
			serializer->read(v.material[1]);
			serializer->read(v.material[2]);
			serializer->read(v.material[3]);
			serializer->read(v.uv[0]);
			serializer->read(v.uv[1]);
			return v;
		}
	};

	std::vector<unsigned int> mIndicesDetailed;
	std::vector<unsigned int> mIndicesSimplified;
	std::vector<PlanetPageVertex> mVertices;
	std::vector<unsigned int> mBorderIndices;

	unsigned long mCenterIndex;
	unsigned long mCornerIndex[4];
	btVector3 mCenterDirection1;
	float mDotToCenterLimit;

	std::unique_ptr<PhysicsBody> mPhysicsBody;

	GLuint mVbo;
	GLuint mIboDetailed;
	GLuint mIboSimplified;

	void setFieldOfView();
	void calculateNormals(bool reset);
	void setVertex(const std::string& plane, float radius, float d1, float d2, float face, PlanetPageVertex& vertex);
	void buildDetailedMesh(const std::string& plane, float radius, float d1, float d2, float size, float face, unsigned int pageDivisions);
	void buildSimplifiedMesh(const std::string& plane, unsigned int pageDivisions);
	void bind();

	PlanetPage(unsigned int pageId);
public:
	PlanetPage(unsigned int pageId, const std::string& plane, float radius, float waterLevel, float a, float b, float size, float face, unsigned int pageDivisions);
	~PlanetPage();

	static btVector3 getInnerPoint(const ICamera*, float planetRadius);
	btVector3 getCorner(unsigned int index) const noexcept;
	float getHeightAt(const btVector3& direction) const;
	void getNormalAt(const btVector3& point, btVector3& normal) const;
	float arcDistanceTo(const btVector3& point) const;
	bool isCloseTo(const btVector3& point) const;
	unsigned int getPageId() const noexcept;
	unsigned int getPageId(const btVector3&) const noexcept;

	void enablePhysics(btDynamicsWorld *dynamicsWorld, const std::forward_list<std::shared_ptr<btRigidBody>>& rigidBodies);
	void editVertices(const std::string& command, float value, const btVector3& point3D, float brushSize);
	void fixBorderNormals(const std::function<void(const btVector3&,btVector3&)>& getNormalAt, const btVector3& mouse3d, float brushSize, const ICamera* camera, const btVector3& innerPoint);
	void autoPaintVertices(const std::string& param, const btVector3& point3D, float brushSize);

	bool isVisible(const ICamera* camera, const btVector3& innerPoint) const;
	bool hasWater() const noexcept;
	void initPhysics(btDynamicsWorld* dynamicsWorld);
	void renderOpaque(float distanceToCamera, const GameState& gameState);
	void renderTranslucent(float distanceToCamera, const GameState& gameState);

	void write(ISerializer *serializer) const;
	static std::string serializeID();
	static std::unique_ptr<PlanetPage> create(ISerializer*, std::weak_ptr<btDynamicsWorld>);
};

//-----------------------------------------------------------------------------

inline bool PlanetPage::hasWater() const noexcept
{ return mHasWater; }

inline unsigned int PlanetPage::getPageId() const noexcept
{ return mPageId; }

#endif