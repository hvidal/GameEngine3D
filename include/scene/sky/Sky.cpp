#include "Sky.h"
#include "../../util/Shader.h"
#include "../../util/ShaderUtils.h"
#include "SkyShader.h"


static constexpr const char* vs[] = {
	"attribute vec4 vertex;"

	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 cameraUp;"
	"uniform vec3 cameraRight;"
	"uniform vec3 centerPoint;"

	"varying vec3 worldV;"
	"varying vec4 projectedV;"

	"const float size = 150000.0;"
	"void main() {"
		"vec3 dx = normalize(cameraRight) * vertex.x * size;"
		"vec3 dy = normalize(cameraUp) * vertex.y * size;"
		"worldV = centerPoint + dx + dy;"
		"projectedV = P * V * vec4(worldV, 1.0);"
		"gl_Position = projectedV;"
	"}"
};

static constexpr const char* fs[] = {
	"varying vec3 worldV;"
	"varying vec4 projectedV;"

	,SkyShader::SHARED_FRAGMENT_CODE,

	// PlanetSurfaceReflection
	"uniform bool isRenderingReflection;"

	,ShaderUtils::IN_FRUSTUM,

	"const float PI = 3.1415926535897;"

	"const vec4 WATER_SHALLOW_COLOR = vec4(63, 99, 97, 255) / 255.0;"

	"void main() {"
		"float dotOriginToPoint = sky_dotOriginToPoint(worldV);"
		"bool belowHorizonLimit = !isRenderingReflection && dotOriginToPoint >= dotOriginToWaterLevel;"
		"if (belowHorizonLimit) {"
			"gl_FragColor = WATER_SHALLOW_COLOR;"
			"return;"
		"}"

		"bool aboveHorizonLimit = isRenderingReflection && dotOriginToPoint < dotOriginToHorizon;"
		"if (aboveHorizonLimit)"
			"discard;"

		"if (isRenderingReflection) {"
			"vec3 vCameraToPoint1 = normalize(worldV - cameraPosition);"
			"vec3 vCameraToOrigin = -cameraPosition;"
			// project vCameraToOrigin onto vCameraToPoint1
			"float d = dot(vCameraToOrigin, vCameraToPoint1);"
			// calculate distance of the line to the origin using pitagoras
			"float dCameraOrigin = length(vCameraToOrigin);"
			"float a = sqrt(dCameraOrigin * dCameraOrigin - d * d);"
			// calculate the inner distance using the information above
			"float d_inner = sqrt(planetRadius * planetRadius - a * a);"
			"float distanceToSurfacePoint = d - d_inner;"
			"vec3 surfacePoint = cameraPosition + distanceToSurfacePoint * vCameraToPoint1;"
			"vec3 surfaceNormal1 = normalize(surfacePoint);"
			"vec3 reflectedVector1 = reflect(vCameraToPoint1, surfaceNormal1);"
			"vec3 reflectedPoint = surfacePoint + 25000.0 * reflectedVector1;"

			"float dotOriginToReflectedPoint = sky_dotOriginToPoint(reflectedPoint);"
			"gl_FragColor = sky_color(reflectedPoint, dotOriginToReflectedPoint, true);"
		"} else {"
			"if (!inFrustum(projectedV))"
				"discard;"
			"gl_FragColor = sky_color(worldV, dotOriginToPoint, true);"
		"}"
	"}"
};

static const btVector3 gVertexBuffer[] = {
	btVector3(-0.5f, -0.5f, 0.0f),
	btVector3(0.5f, -0.5f, 0.0f),
	btVector3(-0.5f, 0.5f, 0.0f),
	btVector3(0.5f, 0.5f, 0.0f)
};

static constexpr const unsigned int gIndices[] = {
	0, 1, 2,
	1, 3, 2
};

const std::string& Sky::SERIALIZE_ID = "Sky";

//-----------------------------------------------------------------------------

Sky::Sky(float planetRadius, float waterLevel, const btVector3& sunPosition):
	SceneObject(),
	mAmbient(1.0f),
	mDiffuse(0.5f),
	mSpecular(0.5f),
	mPlanetRadius(planetRadius),
	mWaterLevel(waterLevel),
	mSunPosition(sunPosition)
{
	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "vertex");
	mShader->link();

	bind();
}


Sky::~Sky() {
	glDeleteBuffers(1, &mVbo);
	glDeleteBuffers(1, &mIbo);
}


void Sky::bind() {
	glGenBuffers(1, &mVbo);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(btVector3), &gVertexBuffer[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &mIbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), &gIndices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void Sky::setVars(IShader* shader, const ICamera* camera) const {
	// Calculate the angle between vectors vCameraToOrigin and vCameraToHorizon
	// Since we don't have a horizon point, the vCameraToHorizon vector is unknown.
	// But we know this is a rectangle triangle and we also have two distances
	// (1) distance from origin to horizon = planet radius = opposite side
	// (2) distance from camera to origin = length of camera position = hypotenuse
	// Using trigonometry, sin(angle) = planet_radius / camera_position_length
	float d = camera->getPosition().length();
	float dotOriginToHorizon = 0.f;
	if (d > mPlanetRadius) { // only if above surface
		double horizonAngle = asin(mPlanetRadius / d);
		dotOriginToHorizon = static_cast<float>(cos(horizonAngle));
	}

	float dotOriginToWaterLevel = 0.f;
	if (d > mWaterLevel) { // only if above water level
		double waterLevelAngle = asin(mWaterLevel / d);
		dotOriginToWaterLevel = static_cast<float>(cos(waterLevelAngle));
	}

	// Calculate the angle between vectors vCameraToOrigin and vCameraToSun
	// We don't know how high the sun is above the horizon, so we can't use the approach above
	// We have to find the answer with the dot product between the normalized vectors,
	// which equals cos(angle)
	const btVector3& vCameraToOrigin1 = -camera->getPosition().normalized();
	const btVector3& vCameraToSun1 = (mSunPosition - camera->getPosition()).normalized();
	float dotOriginToSun = vCameraToOrigin1.dot(vCameraToSun1);

	shader->set("cameraPosition", camera->getPosition());
	shader->set("sunPosition", mSunPosition);
	shader->set("planetRadius", mPlanetRadius);
	shader->set("dotOriginToSun", dotOriginToSun);
	shader->set("dotOriginToHorizon", dotOriginToHorizon);
	shader->set("dotOriginToWaterLevel", dotOriginToWaterLevel);
	shader->set("ambientLight", mAmbient);
	shader->set("diffuseLight", mDiffuse);
}


void Sky::renderOpaque(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) {
	if (shadowMap->isRendering())
		return;

	mShader->run();

	IShader* pShader = mShader.get();
	setVars(pShader, camera);
	camera->setMatrices(pShader);
	surfaceReflection->setVars(pShader, camera);
	mShader->set("cameraUp", camera->getUp());
	mShader->set("cameraRight", camera->getRight());
	mShader->set("centerPoint", camera->getPosition() + 25000.0 * camera->getDirectionUnit());

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	IShader::stop();
}


void Sky::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mAmbient);
	serializer->write(mDiffuse);
	serializer->write(mSpecular);
	serializer->write(mPlanetRadius);
	serializer->write(mWaterLevel);
	serializer->write(mSunPosition);
}


const std::string& Sky::serializeID() const noexcept {
	return SERIALIZE_ID;
}

void Sky::debug() const {
	Log::debug("SKY ambient=%.2f diffuse=%.2f specular=%.2f POSITION %.2f %.2f %.2f", mAmbient, mDiffuse, mSpecular, mSunPosition.x(), mSunPosition.y(), mSunPosition.z());
}

std::pair<std::string, Factory> Sky::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		float ambient, diffuse, specular, planetRadius, waterLevel;
		serializer->read(ambient);
		serializer->read(diffuse);
		serializer->read(specular);
		serializer->read(planetRadius);
		serializer->read(waterLevel);

		btVector3 sunPosition;
		serializer->read(sunPosition);

		std::shared_ptr<Sky> o = std::make_shared<Sky>(planetRadius, waterLevel, sunPosition);
		o->setObjectId(objectId);
		o->mAmbient = ambient;
		o->mDiffuse = diffuse;
		o->mSpecular = specular;

		IGameScene* gameScene = window->getGameScene().get();
		gameScene->setSky(o);
		gameScene->addSceneObject(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}
