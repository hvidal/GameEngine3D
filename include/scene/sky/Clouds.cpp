#include "Clouds.h"
#include "../../util/Shader.h"
#include "../../util/Texture.h"
#include "../planet/SurfaceReflection.h"
#include "SkyShader.h"
#include "../../util/IoUtils.h"

static const char* vs[] = {
	"attribute vec4 vertex;"
	"attribute vec4 position;"
	"attribute vec3 dimension;"

	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 cameraPosition;"

	"varying vec3 worldV;"
	"varying vec3 centerV;"
	"varying vec4 eyeV;"
	"varying vec4 uv;"

	,SurfaceReflection::getVertexShaderCode(),

	"void main() {"
		"vec3 position0 = planetSurfaceReflection(position.xyz);"

		"vec3 normal = normalize(position0.xyz);"
		"vec3 toCloud = position0 - cameraPosition;"
		"vec3 left1 = normalize(cross(normal, toCloud));"
		"vec3 up1 = normalize(cross(toCloud, left1));"
		"if (isRenderingReflection) "
			"up1 = -up1;"

		"vec3 right1 = -left1;"
		"vec3 dx = right1 * vertex.x * dimension.x;"
		"vec3 dy = up1 * vertex.y * dimension.y;"
		"uv = vec4(0.5 + vertex.x, 0.5 - vertex.y, dimension.z, 1.5 * (dimension.x + dimension.y));"
		"centerV = position0.xyz;"
		"worldV = centerV + dx + dy;"
		"eyeV = V * vec4(worldV, 1.0);"
		"gl_Position = P * V * vec4(worldV, 1.0);"
	"}"
};

static constexpr const char* fs[] = {
	"uniform sampler2D material0;"
	"uniform sampler2D material1;"
	"uniform sampler2D material2;"
	"uniform sampler2D material3;"

	"varying vec3 worldV;"
	"varying vec3 centerV;"
	"varying vec4 eyeV;"
	"varying vec4 uv;"

	// PlanetSurfaceReflection
	"uniform bool isRenderingReflection;"

	,SkyShader::SHARED_FRAGMENT_CODE,

	"vec4 color() {"
		"return texture2D(uv.z < 0.0? material0 : uv.z < 1.0? material1 : uv.z < 2.0? material2 : material3, uv.xy);"
	"}"

	"void main() {"
		"vec4 c = color();"
		"if (c.a <= 0.001)"
			"discard;"

		"float dotOriginToPoint = sky_dotOriginToPoint(worldV);"
		"bool belowHorizonLimit = !isRenderingReflection && dotOriginToPoint > dotOriginToHorizon;"
		"bool aboveHorizonLimit = isRenderingReflection && dotOriginToPoint < dotOriginToHorizon;"
		"if (belowHorizonLimit || aboveHorizonLimit)"
			"discard;"

		"float alpha = c.a;"
		"c = sky_mixHaze(c, -eyeV.z, worldV);"
		"c.a = alpha;"

		"float darkness = 0.8 * ambientLight;"

		"vec3 vOriginToCamera = normalize(cameraPosition);"
		"vec3 vOriginToSun = normalize(sunPosition);"
		"float _dot = dot(vOriginToCamera, vOriginToSun);"
		"if (_dot < -0.5) {" // night side
			// make clouds darker
			"c = c - vec4(darkness, darkness, darkness, 0.0);"
		"} else {"
			"vec3 vCloudToSun = normalize(sunPosition - centerV);"
			"vec3 vCloudToCenter = worldV - centerV;"
			"float projected = dot(vCloudToCenter, vCloudToSun) / uv.w;"
			"vec4 brightness = vec4(projected, projected, projected, 0.0);"
			"if (_dot < 0.5) {" // horizon zone
				"float factor = _dot + 0.5;" // 1.0 .. 0.0
				"float t = darkness - factor * darkness;"
				"c = c - vec4(t, t, t, 0.0) + factor * brightness;"
			"} else "
				"c = c + brightness;"
		"}"
		"gl_FragColor = c;"
	"}"
};

static constexpr const GLfloat gVertexBuffer[] = {
	-0.5f,	-0.5f,	0.0f,
	0.5f,	-0.5f,	0.0f,
	-0.5f,	0.5f,	0.0f,
	0.5f, 	0.5f, 	0.0f
};

const std::string& Clouds::SERIALIZE_ID = "Clouds";

//-----------------------------------------------------------------------------

Clouds::Clouds(const std::pair<float,float>& altitudeRange):
	mAltitudeRange(altitudeRange)
{
	mTexture[0] = std::make_unique<Texture>(1, IoUtils::resource("/texture/cloudA.png"));
	mTexture[1] = std::make_unique<Texture>(2, IoUtils::resource("/texture/cloudB.png"));
	mTexture[2] = std::make_unique<Texture>(3, IoUtils::resource("/texture/cloudC.png"));
	mTexture[3] = std::make_unique<Texture>(4, IoUtils::resource("/texture/cloudD.png"));
	mTexture[0]->mipmap()->repeat();
	mTexture[1]->mipmap()->repeat();
	mTexture[2]->mipmap()->repeat();
	mTexture[3]->mipmap()->repeat();

	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "vertex");
	mShader->bindAttribute(1, "position");
	mShader->bindAttribute(2, "dimension");
	mShader->link();
}


Clouds::~Clouds() {
	Log::debug("Deleting clouds");
	mParticles.clear();
	glDeleteBuffers(1, &mVbo);
	glDeleteBuffers(1, &mPbo);
}


void Clouds::bind() {
	// The VBO containing the 4 vertices of the particles.
	// Thanks to instancing, they will be shared by all particles.
	glGenBuffers(1, &mVbo);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVertexBuffer), &gVertexBuffer[0], GL_STATIC_DRAW);

	// The VBO containing the positions of the particles
	glGenBuffers(1, &mPbo);
	glBindBuffer(GL_ARRAY_BUFFER, mPbo);
	glBufferData(GL_ARRAY_BUFFER, mParticles.size() * sizeof(Particle), &mParticles[0].position, GL_STREAM_DRAW);
}


const static auto random0 = []() -> float {
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
};


void Clouds::addCloud(float planetRadius, const btVector3& direction, int particles, float width, float height) {
	float altitude = planetRadius + mAltitudeRange.first + random0() * (mAltitudeRange.second - mAltitudeRange.first);
	btVector3 base = altitude * direction;

	const btVector3& up = base.normalized();
	for (int c = 0; c < particles; c++) {
		Particle particle(base);
		particle.dimension[0] = width;
		particle.dimension[1] = height;
		particle.dimension[2] = static_cast<float>(c % 4) - .1f;
		mParticles.push_back(particle);

		// move base to a random place nearby
		float x = 2.f * random0() - 1.f;
		float y = 2.f * random0() - 1.f;
		float z = 2.f * random0() - 1.f;
		const btVector3& randomDirection = btVector3(x, y, z).cross(up);
		base += 0.1f * width * randomDirection.normalized();
	}
}


void Clouds::createClouds(float planetRadius, float width, float height, unsigned int cloudCount) {
	for (int c = 0; c < cloudCount; c++) {
		float x = 2.f * random0() - 1.f;
		float y = 2.f * random0() - 1.f;
		float z = 2.f * random0() - 1.f;
		const btVector3& direction = btVector3(x, y, z).normalized();
		int particles = static_cast<int> (2 + 5 * random0());
		addCloud(planetRadius, direction, particles, width, height);
	}
}


void Clouds::renderOpaque(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) {
	// nothing opaque to render
}


void Clouds::renderTranslucent(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) {
	if (shadowMap->isRendering())
		return;

	// Sort particles so that we can render them back to front
	// Without this step clouds don't look so realistic when we fly through them
	for (Particle& particle : mParticles) {
		particle.cameraDistance = camera->getPosition().distance(particle.position);
	}
	std::sort(mParticles.begin(), mParticles.end());

	IShader* pShader = mShader.get();

	mShader->run();
	camera->setMatrices(pShader);
	sky->setVars(pShader, camera);
	surfaceReflection->setVars(pShader, camera);
	mShader->set("material0", mTexture[0].get());
	mShader->set("material1", mTexture[1].get());
	mShader->set("material2", mTexture[2].get());
	mShader->set("material3", mTexture[3].get());

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, mPbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), 0);
	glBufferData(GL_ARRAY_BUFFER, mParticles.size() * sizeof(Particle), &mParticles[0].position, GL_STREAM_DRAW);

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ARRAY_BUFFER, mPbo);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), (GLvoid*) offsetof(Particle, dimension));

	glVertexAttribDivisor(0, 0); // particles vertices : always reuse the same 4 vertices -> 0
	glVertexAttribDivisor(1, 1); // positions : one per quad (its center)                 -> 1
	glVertexAttribDivisor(2, 1); // dimensions : one per quad (its center)                -> 1

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, mParticles.size());

	glVertexAttribDivisor(1, 0); // disable
	glVertexAttribDivisor(2, 0); // disable

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	IShader::stop();
}


void Clouds::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mAltitudeRange.first);
	serializer->write(mAltitudeRange.second);
	serializer->write(mParticles.size());
	for (const Particle& p : mParticles) {
		p.write(serializer);
	}
}


std::pair<std::string, Factory> Clouds::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		std::vector<Particle>::size_type cloudCount;

		float minAlt, maxAlt;
		serializer->read(minAlt);
		serializer->read(maxAlt);
		serializer->read(cloudCount);

		std::shared_ptr<Clouds> o = std::make_shared<Clouds>(std::make_pair(minAlt, maxAlt));
		o->setObjectId(objectId);
		o->mParticles.reserve(cloudCount);
		for (size_type i = 0; i < cloudCount; i++) {
			o->mParticles.push_back(Particle::read(serializer));
		}
		o->bind();
		window->getGameScene()->addSceneObject(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& Clouds::serializeID() const noexcept {
	return SERIALIZE_ID;
}
