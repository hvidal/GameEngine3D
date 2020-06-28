#include <set>
#include "Grass.h"
#include "../../util/IoUtils.h"
#include "../../util/ShaderNoise.h"
#include "../../util/ShadowMap.h"
#include "../../util/Texture.h"
#include "../../util/ShaderUtils.h"


static constexpr const char* vs[] = {
	"attribute vec3 vertex;"
	"attribute vec2 uv;"
	"attribute vec3 position;"
	"attribute float rotation;"

	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 cameraPosition;"

	"varying vec3 worldV;"
	"varying vec2 _uv;"
	"varying float _distance;"
	"varying float _height;"

	,ShaderUtils::TO_PLANET_SURFACE,

	"vec3 rotateY(vec3 v, float angle) {"
		"float _sin = sin(angle);"
		"float _cos = cos(angle);"
		"mat3 rot = mat3("
			"_cos, 0, _sin,"
			"0, 1, 0,"
			"-_sin, 0, _cos"
		");"
		"return rot * v;"
	"}"

	,ShadowMap::getVertexShaderCode(),
	""
	,ShaderNoise::SHADER_CODE,

	"const vec3 DOWN1 = vec3(0.0, -1.0, 0.0);"

	"void main() {"
		"_uv = uv;"
		"_height = vertex.y;"
		"_distance = distance(position, cameraPosition);"
		"vec3 noise = vertex.y > 0.25 && _distance < 100.0? noiseNormal(vertex) + DOWN1 : vec3(0.0);"

		"worldV = position + toPlanetSurface(position, noise + rotateY(vertex, rotation));"

		"vec4 worldV_4 = vec4(worldV, 1.0);"
		"setShadowMap(worldV_4);"
		"gl_Position = P * V * worldV_4;"
	"}"
};

static constexpr const char* fs[] = {
	"varying vec3 worldV;"
	"varying vec2 _uv;"
	"varying float _distance;"
	"varying float _height;"

	"uniform sampler2D texture;"

	,ShadowMap::getFragmentShaderCode(),

	"float occlusion() {"
		"if (_height > 1.0) "
			"return 1.0;"
		"return 0.8 + 0.2 * clamp(_height, 0.0, 1.0);"
	"}"

	"vec4 color() {"
		"return texture2D(texture, _uv);"
	"}"

	"void main() {"
		"if (_uv.y <= 0.05 || _distance > 1000.0)"
			"discard;" // get rid of some strange artifacts
		"vec4 c = color();"
		"if (c.a <= 0.5)"
			"discard;"
		"float shadow = _distance > 300.0? 0.8 : max(getShadow9(worldV), 0.4);"
		"gl_FragColor = vec4(c.xyz * shadow * occlusion(), c.a);"
	"}"
};

const std::string& Grass::SERIALIZE_ID = "Grass";

//-----------------------------------------------------------------------------

Grass::Grass(std::weak_ptr<Planet> planet, const std::string& filename):
	mPlanet(planet),
	mFilename(filename),
	mGrassMode(false)
{
	mModel = std::make_unique<ModelOBJ>();
	auto fullpath = IoUtils::resource(mFilename);
	if (!mModel->import(fullpath.c_str())) {
		Log::error("Failed to load grass model: %s", mFilename.c_str());
		exit(-1);
	}

	auto texturePath = IoUtils::resource("/model/vegetation/" + mModel->getMesh(0).pMaterial->colorMapFilename);
	mTexture = std::make_unique<Texture>(1, texturePath);
	mTexture->mipmap()->bind();

	mPin = std::make_unique<Pin>();

	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "vertex");
	mShader->bindAttribute(1, "uv");
	mShader->bindAttribute(2, "position");
	mShader->bindAttribute(3, "rotation");
	mShader->link();

	// The VBO containing the shared vertices.
	// Thanks to instancing, they will be shared by all instances.
	glGenBuffers(1, &mVbo);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, mModel->getNumberOfVertices() * mModel->getVertexSize(), &mModel->getVertexBuffer()->position[0], GL_STATIC_DRAW);

	glGenBuffers(1, &mIboSimple);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIboSimple);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * mModel->getMesh(1).triangleCount * mModel->getIndexSize(), mModel->getIndexBuffer() + mModel->getMesh(1).startIndex, GL_STATIC_DRAW);

	glGenBuffers(1, &mIboDetailed);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIboDetailed);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * mModel->getMesh(0).triangleCount * mModel->getIndexSize(), mModel->getIndexBuffer() + mModel->getMesh(0).startIndex, GL_STATIC_DRAW);

	glGenBuffers(1, &mPboTemp);
}

Grass::~Grass() {
	Log::debug("Deleting Grass");
	glDeleteBuffers(1, &mVbo);
	glDeleteBuffers(1, &mIboSimple);
	glDeleteBuffers(1, &mIboDetailed);
	glDeleteBuffers(1, &mPboTemp);
}


ISceneObject::CommandMap Grass::getCommands() {
	ISceneObject::CommandMap map;

	map["grass"] = [this](const std::string& param) {
		mGrassMode = true;
		Log::debug("Plant mode on");
	};

	return map;
}

const static btVector3 gY(0.f, 1.0, 0.f);
constexpr const static float _2PI = 6.28318530717959f;

void Grass::addGrass(const btVector3 &point) {
	const btVector3& up = point.normalized();
	const btVector3& left = gY.cross(up).normalized();

	std::shared_ptr<Planet> planet = mPlanet.lock();

	float brushSize = planet->getBrushSize();
	static std::set<unsigned int> modifiedPageIds;
	modifiedPageIds.clear();

	static const auto randomValue = [] {
		return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	};

	auto count = 2 * brushSize;
	for (int i = 0; i < count; i++) {
		const btVector3& direction = left.rotate(up, randomValue() * _2PI);
		float random2 = randomValue();
		float length = -4.f * random2 * random2 + 4.f * random2; // parabola (upside down / peak at x = 0.5)
		const btVector3& shift = length * brushSize * direction;

		GrassData data;
		data.position = planet->getSurfacePoint(point + shift);
		data.rotation = _2PI * randomValue();

		unsigned int pageId = planet->getPageId(data.position);
		if (!mPagePoints[pageId]) {
			mPagePoints[pageId] = std::make_unique<GrassPageData>();
		}
		mPagePoints[pageId]->points.push_back(data);
		modifiedPageIds.insert(pageId);
	}
	Log::debug("Added grass | page ID %d");
	for (auto&& pageId : modifiedPageIds) {
		auto& data = mPagePoints[pageId];
		data->update();
		data->bind();
		Log::debug("middle point[%u] = %.2f %.2f %.2f", pageId, data->middlePoint.x(), data->middlePoint.y(), data->middlePoint.z());
	}
}


void Grass::removeGrass(const btVector3 &point) {
	Log::debug("Removing grass around %.2f, %.2f, %.2f", point.x(), point.y(), point.z());

	std::shared_ptr<Planet> planet = mPlanet.lock();
	auto brushSize = planet.get()->getBrushSize();

	for (auto& iterator : mPagePoints) {
		std::vector<GrassData>& points = iterator.second->points;
		std::vector<GrassData> toBeRemoved;
		toBeRemoved.reserve(points.size());
		for (GrassData& data : points) {
			if (data.position.distance(point) <= brushSize)
				toBeRemoved.push_back(data);
		}
		std::vector<GrassData> clean;
		clean.reserve(points.size() - toBeRemoved.size());
		for (GrassData& data : points) {
			auto found = std::find(toBeRemoved.cbegin(), toBeRemoved.cend(), data);
			if (found == toBeRemoved.cend()) {
				clean.push_back(data);
			}
		}
		points.swap(clean);
		iterator.second->bind();
	}
}


void Grass::renderOpaque(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) {
	if (pageIds.empty())
		return;
	else if (gameState.mode == GameMode::EDITING) {
		if (mGrassMode) {
			if (gameState.isLeftMouseDown) {
				addGrass(gameState.mouse3d);
				mGrassMode = false;
			} else if (gameState.isRightMouseDown) {
				removeGrass(gameState.mouse3d);
				mGrassMode = false;
			}
		}

		// Show pins
		static const btVector3 PIN_COLOR(0.1f, 0.7f, 0.1f);
		for (auto& pIt : pageIds) {
			auto iterator = mPagePoints.find(pIt.first);
			if (iterator != mPagePoints.end()) {
				static std::vector<btVector3> points;
				points.clear();
				for (unsigned long i = 0; i < iterator->second->points.size(); ++i)
					points.emplace_back(iterator->second->points[i].position);

				mPin->render(points, camera, sky->getSunPosition(), PIN_COLOR);
			}
		}
	}
}


void Grass::renderTranslucent(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) {
	if (gameState.mode != GameMode::EDITING && !pageIds.empty()) {
		static std::vector<GrassPageData*> visiblePageData(20);
		static std::vector<GrassData> closePoints(50);

		visiblePageData.clear();
		closePoints.clear();

		const btVector3& cameraPosition = camera->getPosition();
		for (auto& pIt : pageIds) {
			if (pIt.second > 1000.0f)
				continue;
			auto it = mPagePoints.find(pIt.first);
			if (it != mPagePoints.end()) {
				visiblePageData.push_back(it->second.get());

				for (auto& v : it->second->points) {
					float d = cameraPosition.distance(v.position);
					// check minimum distance to camera
					if (d <= 100.0f) {
						// If distance is too close or if point is visible to the camera,
						// then render it with details.
						if (d <= 30.0f || camera->isVisible(v.position))
							closePoints.push_back(v);
					}
				}
			}
		}

		if (visiblePageData.size() > 0) {
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_TRUE); // Make depth buffer read write

			render(visiblePageData, closePoints, camera, sky, shadowMap);

			glDepthMask(GL_FALSE); // Make depth buffer read only
			glEnable(GL_CULL_FACE);
		}
	}
}


void Grass::render(const std::vector<GrassPageData*>& visiblePageData, const std::vector<GrassData>& closePoints, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap) {
	IShader* pShader = mShader.get();

	pShader->run();
	sky->setVars(pShader, camera);
	ShaderNoise::setVars(pShader);

	camera->setMatrices(pShader);
	shadowMap->setVars(pShader);
	pShader->set("texture", mTexture.get());

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glVertexAttribDivisor(0, 0); // vertices : always reuse the same vertices	-> 0
	glVertexAttribDivisor(1, 0); // uv : always reuse the same uv 				-> 0
	glVertexAttribDivisor(2, 1); // positions : one per quad (its center)		-> 1
	glVertexAttribDivisor(3, 1); // rotation : one per quad 					-> 1

	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, mModel->getVertexSize(), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, mModel->getVertexSize(), (GLvoid*) offsetof(ModelOBJ::Vertex, texCoord));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIboSimple);

	const unsigned int simple_IndexCount = 3 * mModel->getMesh(1).triangleCount;

	// Render simplified mesh in all visible points
	for (auto& data : visiblePageData) {
		glBindBuffer(GL_ARRAY_BUFFER, data->pbo);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GrassData), 0);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GrassData), (GLvoid*) offsetof(GrassData, rotation));
		glDrawElementsInstanced(GL_TRIANGLES, simple_IndexCount, GL_UNSIGNED_INT, 0, data->points.size());
	}

	// Render second mesh for points close to the camera
	if (closePoints.size() > 0) {
		const unsigned int detailed_IndexCount = 3 * mModel->getMesh(0).triangleCount;

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIboDetailed);

		glBindBuffer(GL_ARRAY_BUFFER, mPboTemp);
		glBufferData(GL_ARRAY_BUFFER, closePoints.size() * sizeof(GrassData), &closePoints[0].position, GL_STREAM_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GrassData), 0);
		glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(GrassData), (GLvoid*) offsetof(GrassData, rotation));

		glDrawElementsInstanced(GL_TRIANGLES, detailed_IndexCount, GL_UNSIGNED_INT, 0, closePoints.size());
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glVertexAttribDivisor(2, 0); // disable
	glVertexAttribDivisor(3, 0); // disable

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	IShader::stop();
}


void Grass::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mFilename);
	serializer->write(mPagePoints.size());
	for (auto& it : mPagePoints) {
		serializer->write(it.first);
		it.second->write(serializer);
	}
}


std::pair<std::string, Factory> Grass::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(window->getGameScene()->getSerializable(Planet::SERIALIZE_ID));

		std::string filename;
		serializer->read(filename);

		std::shared_ptr<Grass> o = std::make_shared<Grass>(planet, filename);
		o->setObjectId(objectId);

		GrassPageMap::size_type mapSize;
		serializer->read(mapSize);

		unsigned int pageId;
		for (GrassPageMap::size_type i = 0; i < mapSize; ++i) {
			serializer->read(pageId);
			o->mPagePoints[pageId] = GrassPageData::read(serializer);
			o->mPagePoints[pageId]->bind();
		}

		window->getGameScene()->addSerializable(o);
		window->getGameScene()->addCommands(o->getCommands());
		planet->addExternalObject(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& Grass::serializeID() const noexcept {
	return SERIALIZE_ID;
}






