#ifndef GAMEDEV3D_GRASS_H
#define GAMEDEV3D_GRASS_H

#include "../planet/IPlanetExternalObject.h"
#include "../planet/Planet.h"
#include "../../util/Pin.h"
#include "../../util/ModelOBJ.h"


class Grass: public IPlanetExternalObject {
	std::string mFilename;

	std::weak_ptr<Planet> mPlanet;
	bool mGrassMode;
	GLuint mVbo, mIboSimple, mIboDetailed, mPboTemp;

	std::unique_ptr<Pin> mPin;
	std::unique_ptr<IShader> mShader;
	std::unique_ptr<ModelOBJ> mModel;
	std::unique_ptr<ITexture> mTexture;

	struct GrassData {
		btVector3 position;
		float rotation;

		bool operator==(const GrassData& d) const
		{ return position == d.position;  }

		void write(ISerializer* serializer) const {
			serializer->write(position);
			serializer->write(rotation);
		}

		static GrassData read(ISerializer* serializer) {
			GrassData o;
			serializer->read(o.position);
			serializer->read(o.rotation);
			return o;
		}
	};

	struct GrassPageData {
		std::vector<GrassData> points;
		btVector3 middlePoint;
		GLuint pbo {0}; // position buffer object

		~GrassPageData() {
			glDeleteBuffers(1, &pbo);
		}

		void bind() {
			if (pbo == 0) {
				glGenBuffers(1, &pbo);
			}
			glBindBuffer(GL_ARRAY_BUFFER, pbo);
			glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(GrassData), &points[0].position, GL_STREAM_DRAW);
		}

		void update() {
			middlePoint.setZero();
			for (auto& data : points) {
				middlePoint += data.position;
			}
			middlePoint /= points.size();
		}

		void write(ISerializer* serializer) const {
			serializer->write(middlePoint);
			serializer->write(points.size());
			for (auto& data : points)
				data.write(serializer);
		}

		static std::unique_ptr<GrassPageData> read(ISerializer* serializer) {
			btVector3 middlePoint;
			serializer->read(middlePoint);
			unsigned long vectorSize;
			serializer->read(vectorSize);
			auto o = std::make_unique<GrassPageData>();
			o->middlePoint = middlePoint;
			o->points.reserve(vectorSize);
			for (unsigned long i = 0; i < vectorSize; ++i) {
				o->points.push_back(GrassData::read(serializer));
			}
			return o;
		}
	};

	using GrassPageMap = std::unordered_map<unsigned int, std::unique_ptr<GrassPageData>>;
	GrassPageMap mPagePoints;

	void addGrass(const btVector3& point);
	void removeGrass(const btVector3& point);
	void render(const std::vector<GrassPageData*>& visiblePageData, const std::vector<GrassData>& closePoints, const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap);
public:
	static const std::string& SERIALIZE_ID;

	Grass(std::weak_ptr<Planet> planet, const std::string& filename);
	virtual ~Grass();

	virtual ISceneObject::CommandMap getCommands();

	virtual void renderOpaque(const std::unordered_map<unsigned int,float>& pageIds, const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const GameState& gameState) override;
	virtual void renderTranslucent(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

#endif //GAMEDEV3D_GRASS_H
