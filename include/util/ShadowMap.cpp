#include "ShadowMap.h"
#include "Texture.h"
#include "math/Matrix4x4.h"

const std::string& ShadowMap::SERIALIZE_ID = "ShadowMap";

//-----------------------------------------------------------------------------

ShadowMap::ShadowMap(GLuint slot, unsigned int size) {
	mDepthTexture = std::make_unique<Texture>(slot, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT, size, size, std::vector<Uint32>());
	mDepthTexture->linear()->clampToEdge()->image()->unbind();

	mFBO = std::make_unique<FrameBuffer>();
	FrameBuffer::end();

	static constexpr const float range = 200.0f;
	mProjectionMatrix.ortho(-range, range, -range, range, 0.f, 10000.f);
}


ShadowMap::~ShadowMap() {
	Log::debug("Deleting ShadowMap");
}


void ShadowMap::start(const btVector3& lightPosition, const btVector3& targetPosition, const btVector3& up) {
	mIsRendering = true;
	glUseProgram(0);
	mFBO->startDepth(mDepthTexture.get());

	mViewMatrix.lookAt(lightPosition, targetPosition, up);

	glCullFace(GL_FRONT);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}


void ShadowMap::end() {
	setTextureMatrix();

	mFBO->end();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glCullFace(GL_BACK);
	mIsRendering = false;
}


void ShadowMap::setVars(IShader* shader) const {
	shader->set("shadowMVP", mShadowMVP);
	shader->set("shadowMap", mDepthTexture.get());
	shader->set("shadowSize", static_cast<float>(mDepthTexture->getWidth()));
}


void ShadowMap::setMatrices(IShader *shader) const {
	shader->set("V", mViewMatrix);
	shader->set("P", mProjectionMatrix);
}


void ShadowMap::setTextureMatrix()
{
	// Moving from unit cube [-1,1] to [0,1]
	static constexpr const float bias[16] = {
			0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, 0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 0.5f, 0.0f,
			0.5f, 0.5f, 0.5f, 1.0f
	};
	mShadowMVP = std::move(Matrix4x4(bias).multiplyRight(mProjectionMatrix).multiplyRight(mViewMatrix));
}


void ShadowMap::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mDepthTexture->getSlot());
	serializer->write(mDepthTexture->getWidth());
}


std::pair<std::string, Factory> ShadowMap::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		unsigned int slot, width;
		serializer->read(slot);
		serializer->read(width);

		std::shared_ptr<ShadowMap> o = std::make_shared<ShadowMap>(slot, width);
		o->setObjectId(objectId);

		IGameScene* gameScene = window->getGameScene().get();
		gameScene->setShadowMap(o);
		gameScene->addSerializable(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& ShadowMap::serializeID() const noexcept {
	return SERIALIZE_ID;
}
