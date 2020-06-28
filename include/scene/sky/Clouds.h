#ifndef GAMEDEV3D_CLOUDS_H
#define GAMEDEV3D_CLOUDS_H

#include "../SceneObject.h"


class Clouds: public SceneObject {

	struct Particle {
		btVector3 position;
		float dimension[3]; // width, height, textureNumber
		float cameraDistance;

		Particle(const btVector3& v) {
			position = v;
		}

		bool operator<(const Particle& p) const {
			// Sort in reverse order : far particles drawn first.
			return this->cameraDistance > p.cameraDistance;
		}

		void write(ISerializer* serializer) const {
			serializer->write(position);
			serializer->write(dimension[0]);
			serializer->write(dimension[1]);
			serializer->write(dimension[2]);
		}

		static Particle read(ISerializer* serializer) {
			btVector3 position;
			serializer->read(position);
			Particle p(std::move(position));
			serializer->read(p.dimension[0]);
			serializer->read(p.dimension[1]);
			serializer->read(p.dimension[2]);
			return p;
		}
	};
	std::vector<Particle> mParticles;
	std::pair<float,float> mAltitudeRange;

	std::unique_ptr<ITexture> mTexture[4];
	std::unique_ptr<IShader> mShader;
	GLuint mVbo; // vertex buffer object
	GLuint mPbo; // position buffer object

	void addCloud(float, const btVector3&, int, float, float);
public:
	static const std::string& SERIALIZE_ID;

	Clouds(const std::pair<float,float>&);
	virtual ~Clouds();

	void bind();
	virtual void createClouds(float, float, float, unsigned int);
	virtual void renderOpaque(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) override;
	virtual void renderTranslucent(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();

};


#endif
