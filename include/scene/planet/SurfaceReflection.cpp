#include "SurfaceReflection.h"
#include "../../util/Texture.h"
#include "../../util/ShaderUtils.h"

const char* SurfaceReflection::getVertexShaderCode() {
	static const std::string gVertexShaderCode =
		"uniform bool isRenderingReflection;"
		"uniform float waterLevel;"
		"uniform float distanceToHorizon;"
		"uniform float dotOriginToHorizon;"

		+ std::string(ShaderUtils::ROTATION_MATRIX) +

		"vec3 planetSurfaceReflection(vec3 p) {"
			"if (!isRenderingReflection)"
				"return p;"
			// objects near the camera should be reflected on the ground
			"if (distance(p, cameraPosition) <= distanceToHorizon) {"
				"vec3 surfacePoint = waterLevel * normalize(p);"
				"vec3 r = surfacePoint - p;"
				"return p + 2.0 * r;"
			"} else {"
				// for objects beyond the horizon, the mirror is the horizon line
				"vec3 vCameraToPoint = p - cameraPosition;"
				"vec3 vCameraToOrigin1 = normalize(-cameraPosition);"
				"vec3 left = cross(vCameraToPoint, vCameraToOrigin1);"
				"mat3 rot = rotationMatrix(left, dotOriginToHorizon);"
				"vec3 vCameraToHorizon1 = rot * vCameraToOrigin1;"

				"vec3 horizonPoint = cameraPosition + vCameraToHorizon1 * distanceToHorizon;"
				"vec3 vHorizonUp1 = normalize(horizonPoint);"
				"vec3 vPointToHorizon = horizonPoint - p;"
				"vec3 vReflected = reflect(vPointToHorizon, vHorizonUp1);"
				"return horizonPoint - vReflected;"
			"}"
			"return p;" // hack, for now
		"}";

	return gVertexShaderCode.c_str();
}

const std::string& SurfaceReflection::SERIALIZE_ID = "SurfaceReflection";

//-----------------------------------------------------------------------------

SurfaceReflection::SurfaceReflection(float planetRadius, float waterLevel, unsigned int textureWidth, unsigned int textureHeight):
	mPlanetRadius(planetRadius),
	mWaterLevel(waterLevel)
{
	mColorTexture = std::make_unique<Texture>(0, GL_RGB, GL_RGB, textureWidth, textureHeight, std::vector<Uint32>());
	mColorTexture->mipmap()->repeat()->unbind();

	mDepthTexture = std::make_unique<Texture>(1, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, textureWidth, textureHeight, std::vector<Uint32>());
	mDepthTexture->mipmap()->repeat()->unbind();

	mBuffer = std::make_unique<FrameBuffer>();
	mBuffer->end();
}


SurfaceReflection::~SurfaceReflection() {
	Log::debug("Deleting Surface Reflection");
}


void SurfaceReflection::begin() {
	mIsRendering = true;
	mBuffer->startColorAndDepth(mColorTexture.get(), mDepthTexture.get());
}


void SurfaceReflection::end() {
	mBuffer->end();
	mColorTexture->linear(); // build image
	mIsRendering = false;
}

void SurfaceReflection::setVars(IShader *shader, const ICamera *camera) const {
	float d = camera->getPosition().length();
	shader->set("cameraPosition", camera->getPosition());
	shader->set("isRenderingReflection", mIsRendering);
	shader->set("waterLevel", mWaterLevel);
	shader->set("distanceToHorizon", (float) sqrt(d * d - mPlanetRadius * mPlanetRadius)); // pitagoras
}

void SurfaceReflection::setReflectionTexture(IShader *shader) const {
	shader->set("reflectionTexture", mColorTexture.get());
}

void SurfaceReflection::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mPlanetRadius);
	serializer->write(mWaterLevel);
	serializer->write(mColorTexture->getWidth());
	serializer->write(mColorTexture->getHeight());
}

std::pair<std::string,Factory> SurfaceReflection::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window){
		float planetRadius, waterLevel;
		unsigned int width, height;
		serializer->read(planetRadius);
		serializer->read(waterLevel);
		serializer->read(width);
		serializer->read(height);

		std::shared_ptr<SurfaceReflection> o = std::make_shared<SurfaceReflection>(planetRadius, waterLevel, width, height);
		o->setObjectId(objectId);

		IGameScene* gameScene = window->getGameScene().get();
		gameScene->setSurfaceReflection(o);
		gameScene->addSerializable(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}

const std::string& SurfaceReflection::serializeID() const noexcept {
	return SERIALIZE_ID;
}