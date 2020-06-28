#ifndef GAMEDEV3D_PLANT_H
#define GAMEDEV3D_PLANT_H

#include <unordered_map>
#include "../planet/Planet.h"
#include "../planet/IPlanetExternalObject.h"
#include "../../util/Pin.h"


class Plant: public IPlanetExternalObject {

	std::weak_ptr<Planet> mPlanet;
	bool mPlantMode;

	std::unique_ptr<IShader> mShader;
	std::unique_ptr<IShader> mShadowShader;
	std::array<std::unique_ptr<ITexture>,4> mTextures;

	GLuint mVbo; // vertex buffer object (shared)

	std::unique_ptr<Pin> mPin;

	struct PlantData {
		btVector3 position;
		btVector3 info; // (side, rotation, texture)

		bool operator==(const PlantData& d) const
		{ return position == d.position;  }

		void write(ISerializer* serializer) const {
			serializer->write(position);
			serializer->write(info);
		}

		static PlantData read(ISerializer* serializer) {
			PlantData o;
			serializer->read(o.position);
			serializer->read(o.info);
			return o;
		}
	};

	struct PlantPageData {
		std::vector<PlantData> points;
		GLuint pbo {0}; // position buffer object

		void bind() {
			if (pbo == 0) {
				glGenBuffers(1, &pbo);
			}
			glBindBuffer(GL_ARRAY_BUFFER, pbo);
			glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(PlantData), &points[0].position, GL_STREAM_DRAW);
		}

		void deleteBuffers() {
			glDeleteBuffers(1, &pbo);
		}

		void write(ISerializer* serializer) const {
			serializer->write(points.size());
			for (auto& data : points)
				data.write(serializer);
		}

		static PlantPageData read(ISerializer* serializer) {
			unsigned long vectorSize;
			serializer->read(vectorSize);
			PlantPageData o;
			o.points.reserve(vectorSize);
			for (unsigned long i = 0; i < vectorSize; ++i) {
				o.points.push_back(PlantData::read(serializer));
			}
			return o;
		}
	};

	using PlantPageMap = std::unordered_map<unsigned int, PlantPageData>;
	PlantPageMap mPagePoints;

	void addPlants(const btVector3& point);
	void removePlants(const btVector3& point);
	void render(const std::unordered_map<unsigned int,float>& pageIds, const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap);
public:
	static const std::string& SERIALIZE_ID;

	Plant(std::weak_ptr<Planet> planet);
	virtual ~Plant();

	virtual ISceneObject::CommandMap getCommands();

	virtual void renderOpaque(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) override;
	virtual void renderTranslucent(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

#endif
