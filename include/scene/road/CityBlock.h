#ifndef CITYBLOCK_H
#define CITYBLOCK_H

#include <LinearMath/btVector3.h>
#include <vector>
#include "../planet/Planet.h"
#include "../../util/PhysicsBody.h"


class CityBlock: public ISerializable {

	struct CityBlockVertex {
		btVector3 position;
		btVector3 normal;
		float uv[2];

		void write(ISerializer* serializer) const {
			serializer->write(position);
			serializer->write(normal);
			serializer->write(uv[0]);
			serializer->write(uv[1]);
		}

		static CityBlockVertex read(ISerializer* serializer) {
			CityBlockVertex v;
			serializer->read(v.position);
			serializer->read(v.normal);
			serializer->read(v.uv[0]);
			serializer->read(v.uv[1]);
			return v;
		}
	};

	std::weak_ptr<Planet> mPlanet;

	std::vector<unsigned int> mIndices;
	std::vector<CityBlockVertex> mVertices;

	GLuint mVbo;
	GLuint mIbo;
	std::unique_ptr<IShader> mShader;
	std::shared_ptr<ITexture> mTexture;

	std::shared_ptr<btDynamicsWorld> mDynamicsWorld;
	std::unique_ptr<PhysicsBody> mPhysicsBody;

	CityBlock(std::shared_ptr<btDynamicsWorld> dynamicsWorld, std::weak_ptr<Planet> planet);
public:
	static const std::string& SERIALIZE_ID;

	CityBlock(std::shared_ptr<btDynamicsWorld> dynamicsWorld, std::weak_ptr<Planet> planet, std::unique_ptr<ITexture> texture);
	~CityBlock();

	void add(const btVector3& centerPoint, const std::vector<btVector3>& ccwPoints, bool isOpen);
	void cleanup();
	void initPhysics();
	void bind();
	void render(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState);

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::shared_ptr<CityBlock> read(ISerializer*, std::shared_ptr<IWindow>, std::shared_ptr<btDynamicsWorld>, std::weak_ptr<Planet>);
};

//-----------------------------------------------------------------------------

#endif
