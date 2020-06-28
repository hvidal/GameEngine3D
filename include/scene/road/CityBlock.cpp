#include "CityBlock.h"
#include "../../util/Shader.h"
#include "../../util/ShadowMap.h"
#include "../../util/ShaderUtils.h"
#include "../../util/Texture.h"


static const char* vs[] = {
	"attribute vec4 position;"
	"attribute vec4 normal;"
	"attribute vec2 uv;"

	"uniform mat4 V;"
	"uniform mat4 P;"

	"uniform vec3 lightPosition;"

	,ShadowMap::getVertexShaderCode(),
	""
	,ShaderUtils::TO_MAT3,

	"varying vec3 worldV;"
	"varying vec2 _uv;"
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"

	"void main() {"
		"setShadowMap(position);"

		"worldV = position.xyz;"
		"eyeV = V * position;"
		"eyeN = toMat3(V) * normal.xyz;"
		"eyeL = V * vec4(lightPosition, 1.0);"
		"_uv = uv;"
		"gl_Position = P * V * position;"
	"}"
};

static const char* fs[] = {
	"uniform sampler2D material0;"
	"uniform float material0_scale;"

	"uniform float ambientLight;"
	"uniform float diffuseLight;"

	"varying vec3 worldV;"
	"varying vec2 _uv;"
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"

	,ShadowMap::getFragmentShaderCode(),

	"vec4 color(vec3 eyeV, vec3 eyeN, vec3 eyeL) {"
		"vec3 n = normalize(eyeN);"
		"vec3 s = normalize(eyeL - eyeV);"
		"vec3 v = normalize(-eyeV);"
		"float dotSN = dot(s, n);"
		"float shading = max(dotSN, 0.0);"
		"float occlusion = dotSN < 0.0? 1.0 - abs(dotSN) : 1.0;"
		"float shadow = getShadow9(worldV);"
		"vec4 matColor = texture2D(material0, _uv * material0_scale);"
		"vec4 Ka = matColor * ambientLight * occlusion;"
		"vec4 Kd = matColor * diffuseLight * shading * shadow * occlusion;"
		"return Ka + Kd;"
	"}"

	"void main() {"
		"gl_FragColor = color(eyeV.xyz, eyeN, eyeL.xyz);"
		"gl_FragColor.a = 1.0;"
	"}"
};

const std::string& CityBlock::SERIALIZE_ID = "CityBlock";


/* private constructor */
CityBlock::CityBlock(std::shared_ptr<btDynamicsWorld> dynamicsWorld, std::weak_ptr<Planet> planet):
	mDynamicsWorld(dynamicsWorld),
	mPlanet(planet)
{
	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "position");
	mShader->bindAttribute(1, "normal");
	mShader->bindAttribute(2, "uv");
	mShader->link();
}


CityBlock::CityBlock(std::shared_ptr<btDynamicsWorld> dynamicsWorld, std::weak_ptr<Planet> planet, std::unique_ptr<ITexture> texture):
	CityBlock(dynamicsWorld, planet)
{
	mTexture = std::move(texture);
}


CityBlock::~CityBlock() {
	Log::debug("Deleting CityBlock");
	cleanup();
}


void CityBlock::cleanup() {
	if (!mVertices.empty()) {
		mIndices.clear();
		mVertices.clear();
		glDeleteBuffers(1, &mVbo);
		glDeleteBuffers(1, &mIbo);
	}
}


const static float CURB_HEIGHT = .4f;
const static int BLOCK_DIVISIONS = 3;


void CityBlock::add(const btVector3& centerPoint, const std::vector<btVector3>& ccwPoints, bool isOpen) {
	// Takes all border points in CCW order and triangulate them
	unsigned long indexStart = mVertices.size(); // one more because of the center point added above
	unsigned long size = ccwPoints.size();

	CityBlockVertex vCenter;
	vCenter.position = centerPoint + CURB_HEIGHT * centerPoint.normalized();
	vCenter.normal = vCenter.position.normalized();
	Planet::convertPointToUV(vCenter.position, vCenter.uv);
	mVertices.push_back(vCenter);

	for (unsigned long i = 0; i < size; i++) {
		const btVector3& p = ccwPoints[i];

		// Move the v0 point to the side a little bit so that it gets a different UV than v1 (see below).
		const btVector3& out = (p - vCenter.position).normalized();

		CityBlockVertex v0;
		v0.position = p;
		v0.normal = out;
		Planet::convertPointToUV(v0.position + CURB_HEIGHT * out, v0.uv); // move the the side

		CityBlockVertex v1;
		v1.position = p + CURB_HEIGHT * p.normalized();
		v1.normal = (p.normalized() + out).normalized();
		Planet::convertPointToUV(v1.position, v1.uv);

		mVertices.push_back(v0);
		mVertices.push_back(v1);

		// Calculate the number of steps to the center point
		const btVector3& vStep = (vCenter.position - v1.position) / BLOCK_DIVISIONS;
		int verticesPerCut = BLOCK_DIVISIONS + (isOpen? 2 : 1);

		unsigned long s = 1 + indexStart + verticesPerCut * i;
		unsigned long next = i == size - 1? indexStart + 1 : s + verticesPerCut;

		mIndices.push_back(s);
		mIndices.push_back(next);
		mIndices.push_back(s+1);

		mIndices.push_back(next);
		mIndices.push_back(next+1);
		mIndices.push_back(s+1);

		std::shared_ptr<Planet> planet = mPlanet.lock();

		// Now we have to add more vertices from the curb to the center point
		for (int k = 0; k < BLOCK_DIVISIONS-1; k++) {
			const btVector3& point = v1.position + (k+1) * vStep;
			CityBlockVertex v;
			v.position = planet->getSurfacePoint(point, CURB_HEIGHT);
			v.normal = v.position.normalized();
			Planet::convertPointToUV(v.position, v.uv);
			mVertices.push_back(v);

			mIndices.push_back(s+1+k);
			mIndices.push_back(next+1+k);
			mIndices.push_back(s+2+k);

			mIndices.push_back(next+1+k);
			mIndices.push_back(next+2+k);
			mIndices.push_back(s+2+k);
		}
		if (isOpen) {
			// adjust the normal of the last vertice added in the loop above
			CityBlockVertex& lastVertice = mVertices[mVertices.size()-1];
			lastVertice.normal = (lastVertice.normal - out).normalized();

			// add a point on the ground (like an inner curb)
			CityBlockVertex v;
			v.position = planet->getSurfacePoint(lastVertice.position);
			v.normal = -out;
			Planet::convertPointToUV(v.position - CURB_HEIGHT * out, v.uv);
			mVertices.push_back(v);

			mIndices.push_back(s+BLOCK_DIVISIONS);
			mIndices.push_back(next+BLOCK_DIVISIONS);
			mIndices.push_back(s+BLOCK_DIVISIONS+1);

			mIndices.push_back(next+BLOCK_DIVISIONS);
			mIndices.push_back(next+BLOCK_DIVISIONS+1);
			mIndices.push_back(s+BLOCK_DIVISIONS+1);
		} else {
			// last triangle, close to the center point
			mIndices.push_back(s+BLOCK_DIVISIONS);
			mIndices.push_back(next+BLOCK_DIVISIONS);
			mIndices.push_back(indexStart); // center point
		}
	}
}


void CityBlock::bind() {
	if (mVertices.empty())
		return;

	glGenBuffers(1, &mVbo);
	glGenBuffers(1, &mIbo);

	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(CityBlockVertex), &mVertices[0].position, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), &mIndices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void CityBlock::initPhysics() {
	if (mVertices.empty())
		return;

	int vertexStride = sizeof(CityBlockVertex);
	int indexStride = 3 * sizeof(unsigned int);
	unsigned long triangleCount = mIndices.size() / 3;

	std::unique_ptr<btTriangleIndexVertexArray> puIndexVertexArrays = std::make_unique<btTriangleIndexVertexArray>(
			triangleCount,
			reinterpret_cast<int*> (&mIndices[0]),
			indexStride,
			mVertices.size(),
			const_cast<btScalar*>(&mVertices[0].position.x()),
			vertexStride);

	btVector3 localInertia(0,0,0);

	std::unique_ptr<btMotionState> puMotionState = std::make_unique<btDefaultMotionState>();

	mPhysicsBody = std::make_unique<PhysicsBody>(0.f, std::move(puIndexVertexArrays), std::move(puMotionState), localInertia);
	mPhysicsBody->getCollisionShape()->setMargin(.2f);

	btScalar defaultContactProcessingThreshold(BT_LARGE_FLOAT);
	mPhysicsBody->getRigidBody()->setContactProcessingThreshold(defaultContactProcessingThreshold);

	mDynamicsWorld->addRigidBody(mPhysicsBody->getRigidBody());
}


void CityBlock::render(const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap, const GameState& gameState) {
	if (shadowMap->isRendering() || mVertices.empty())
		return;

	IShader* pShader = mShader.get();

	pShader->run();
	shadowMap->setVars(pShader);
	camera->setMatrices(pShader);
	pShader->set("lightPosition", sky->getSunPosition());
	pShader->set("ambientLight", sky->getAmbientLight());
	pShader->set("diffuseLight", sky->getDiffuseLight());
	pShader->set("material0", mTexture.get());
	pShader->set("material0_scale", mTexture->getScaleFactor());

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CityBlockVertex), 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CityBlockVertex), (GLvoid*) offsetof(CityBlockVertex, normal));

	glEnableVertexAttribArray(2);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(CityBlockVertex), (GLvoid*) offsetof(CityBlockVertex, uv));

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mIndices.size()), GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	IShader::stop();
}


void CityBlock::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());

	serializer->write(mVertices.size());
	for (const CityBlockVertex& v : mVertices) {
		v.write(serializer);
	}
	serializer->write(mIndices);

	mTexture->write(serializer);
}


std::shared_ptr<CityBlock> CityBlock::read(ISerializer *serializer, std::shared_ptr<IWindow> window, std::shared_ptr<btDynamicsWorld> dynamicsWorld, std::weak_ptr<Planet> planet) {
	Log::debug("Reading CityBlock");

	// calling private constructor, cannot call std::make_shared
	std::shared_ptr<CityBlock> o = std::shared_ptr<CityBlock>(new CityBlock(dynamicsWorld, planet));

	std::size_t vertexCount;
	serializer->read(vertexCount);
	o->mVertices.reserve(vertexCount);
	for (int i = 0; i < vertexCount; i++) {
		o->mVertices.push_back(CityBlockVertex::read(serializer));
	}
	serializer->read(o->mIndices);

	auto textureFactory = serializer->getFactory(Texture::SERIALIZE_ID);
	std::string name;
	unsigned long objectId;
	serializer->readBegin(name, objectId);
	std::shared_ptr<ISerializable> tex = textureFactory.create(objectId, serializer, window);
	o->mTexture = std::dynamic_pointer_cast<ITexture>(tex);
	o->mTexture->mipmap()->repeat();

	o->bind();
	o->initPhysics();
	return o;
}


const std::string& CityBlock::serializeID() const noexcept {
	return SERIALIZE_ID;
}

