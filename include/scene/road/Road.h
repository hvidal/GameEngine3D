#ifndef ROAD_H
#define ROAD_H

#include <forward_list>
#include <vector>
#include "../SceneObject.h"
#include "../planet/Planet.h"
#include "../../util/Pin.h"
#include "CityBlock.h"


class Road: public SceneObject {

	typedef struct {
		unsigned int p0;
		unsigned int p1;
	} RoadStretch;

	typedef struct {
		unsigned int pointId;
		unsigned int triangleCount;
		std::vector<unsigned int> indices;
		std::vector<btVector3> vertices;
		std::vector<RoadStretch> stretches;
	} JunctionMesh;

	typedef struct {
		RoadStretch stretch;
		btVector3 start[3];
		btVector3 end[3];
		std::vector<unsigned int> indices;
		std::vector<btVector3> vertices;
	} RoadMesh;

	struct RoadVertex {
		btVector3 position;
		float uv[2];
		float border;

		void write(ISerializer* serializer) const {
			serializer->write(position);
			serializer->write(uv[0]);
			serializer->write(uv[1]);
			serializer->write(border);
		}

		static RoadVertex read(ISerializer* serializer) {
			RoadVertex v;
			serializer->read(v.position);
			serializer->read(v.uv[0]);
			serializer->read(v.uv[1]);
			serializer->read(v.border);
			return v;
		}
	};

	struct Block {
		std::vector<unsigned int> pointIds;
		bool isOpen;

		void write(ISerializer* serializer) const {
			serializer->write(isOpen);
			serializer->write(pointIds);
		}

		static Block read(ISerializer* serializer) {
			Block block;
			serializer->read(block.isOpen);
			serializer->read(block.pointIds);
			return block;
		}
	};

	std::weak_ptr<Planet> mPlanet;

	unsigned int mSelectedPointId;
	std::unordered_map<unsigned int, btVector3> mPoints;
	std::forward_list<RoadStretch> mRoadStretchList;
	std::forward_list<Block> mBlocks;

	std::vector<unsigned int> mIndices;
	std::vector<RoadVertex> mVertices;

	GLuint mVbo;
	GLuint mIbo;
	std::unique_ptr<IShader> mShader;
	std::shared_ptr<ITexture> mTexture;

	std::shared_ptr<CityBlock> mCityBlock;
	std::unique_ptr<Pin> mPin;

	void cleanup();
	void bind();
	void buildRoad();
	void buildCityBlocks(const std::vector<std::shared_ptr<JunctionMesh>>& junctionMeshes, const std::vector<std::shared_ptr<RoadMesh>>& roadMeshes);
	void buildFinalMesh(const std::vector<std::shared_ptr<JunctionMesh>>& junctionMeshes, const std::vector<std::shared_ptr<RoadMesh>>& roadMeshes);

	Road(std::shared_ptr<btDynamicsWorld>, std::weak_ptr<Planet> planet);
public:
	static const std::string& SERIALIZE_ID;

	Road(std::shared_ptr<btDynamicsWorld>, std::weak_ptr<Planet>, std::unique_ptr<ITexture> texture, std::unique_ptr<ITexture> blockTexture);
	virtual ~Road();

	virtual ISceneObject::CommandMap getCommands() override;

	virtual void initPhysics(btTransform transform) override;
	virtual void renderOpaque(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

#endif
