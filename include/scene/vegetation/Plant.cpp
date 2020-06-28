#include "Plant.h"
#include "../../util/Texture.h"
#include "../sky/SkyShader.h"
#include "../../util/ShadowMap.h"
#include "../../util/ShaderNoise.h"
#include "../../util/ShaderUtils.h"
#include "../../util/IoUtils.h"

static constexpr const char* vs[] = {
	"attribute vec4 vertex;"
	"attribute vec3 position;"
	"attribute vec3 info;"

	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 cameraPosition;"

	"varying vec3 worldV;"
	"varying vec3 uv;"
	"varying float _distance;"

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

	"vec3 get_uv() {"
		"float x = vertex.w < 1.5? 0.0 : 1.0; "
		"float y = vertex.y < 0.5? 1.0 : 0.0; "
		"return vec3(x, y, info.z);"
	"}"

	"const vec3 WIND_AMPL = vec3(2.0, 1.0, 2.0);"
	"const vec3 DOWN1 = vec3(0.0, -1.0, 0.0);"

	"void main() {"
		"uv = get_uv();"

		"_distance = distance(cameraPosition, position);"
		"vec3 noise = vertex.y > 0.5 && _distance < 200.0? WIND_AMPL * noiseNormal(position + vertex.xyz) + DOWN1 : vec3(0.0);"

		"worldV = position + toPlanetSurface(position, noise + rotateY(info.x * vertex.xyz, info.y));"

		"vec4 worldV_4 = vec4(worldV, 1.0);"
		"setShadowMap(worldV_4);"
		"gl_Position = P * V * worldV_4;"
	"}"
};

static constexpr const char* fs[] = {
	"varying vec3 worldV;"
	"varying vec3 uv;"
	"varying float _distance;"

	"uniform sampler2D texture0;"
	"uniform sampler2D texture1;"
	"uniform sampler2D texture2;"
	"uniform sampler2D texture3;"

	,ShadowMap::getFragmentShaderCode(),
	""
	,SkyShader::SHARED_FRAGMENT_CODE,

	"float occlusion() {"
		"float o = 1.0 - (uv.y * uv.y * uv.y);"
		"return clamp(o, 0.6, 0.95);"
	"}"

	"vec4 color() {"
		"return texture2D(uv.z < 1.0? texture0 : uv.z < 2.0? texture1 : uv.z < 3.0? texture2 : texture3, uv.xy);"
	"}"

	"void main() {"
		// discard the top layer due to strange artifacts
		"if (uv.y < 0.05)"
			"discard;"
		"vec4 c = color();"
		"if (c.a <= 0.001)"
			"discard;"
		"float shadow = max(getShadow9(worldV), 0.5);"
		"gl_FragColor = vec4(c.xyz * occlusion() * shadow, c.a);"
		"gl_FragColor = sky_mixHaze(gl_FragColor, _distance, worldV);"
	"}"
};

static constexpr const char* fsShadow[] = {
	"varying vec3 uv;"

	"uniform sampler2D texture0;"
	"uniform sampler2D texture1;"
	"uniform sampler2D texture2;"
	"uniform sampler2D texture3;"

	"vec4 color() {"
		"return texture2D(uv.z < 1.0? texture0 : uv.z < 2.0? texture1 : uv.z < 3.0? texture2 : texture3, uv.xy);"
	"}"

	"void main() {"
		"vec4 c = color();"
		"if (c.a < 0.3)"
			"discard;"
		"gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);"
	"}"
};

//-----------------------------------------------------------------------------

static constexpr const float gVertexBuffer_Bush[] = {
	0.029922f, 1.953328f, 1.310717f, 0.0,
	0.374510f, -0.003348f, 1.081194f, 1.0,
	-1.049085f, 1.960837f, -0.373232f, 2.0,
	0.374510f, -0.003348f, 1.081194f, 1.0,
	-0.704497f, 0.004161f, -0.602756f, 3.0,
	-1.049085f, 1.960837f, -0.373232f, 2.0,
	//
	0.981216f, 1.924181f, -0.486160f, 0.0f,
	0.610100f, -0.032494f, -0.669720f, 1.0f,
	0.062856f, 1.931691f, 1.290510f, 2.0f,
	0.610100f, -0.032494f, -0.669720f, 1.0f,
	-0.308261f, -0.024985f, 1.106950f, 3.0f,
	0.062856f, 1.931691f, 1.290510f, 2.0f,
	//
	-1.054276f, 1.960616f, -0.461196f, 0.0f,
	-1.044915f, 0.003941f, -0.047271f, 1.0f,
	0.945687f, 1.968125f, -0.470928f, 2.0f,
	-1.044915f, 0.003941f, -0.047271f, 1.0f,
	0.955047f, 0.011450f, -0.057002f, 3.0f,
	0.945687f, 1.968125f, -0.470928f, 2.0f,
	//
	0.921141f, 1.726353f, 1.160952f, 0.0f,
	0.922589f, 0.007119f, 0.139077f, 1.0f,
	-1.078857f, 1.726353f, 1.158118f, 2.0f,
	0.922589f, 0.007119f, 0.139077f, 1.0f,
	-1.077409f, 0.007119f, 0.136242f, 3.0f,
	-1.078857f, 1.726353f, 1.158118f, 2.0f,
	//
	1.301584f, 1.754014f, 0.463761f, 0.0f,
	0.463838f, 0.001618f, 0.940506f, 1.0f,
	0.312387f, 1.754014f, -1.274482f, 2.0f,
	0.463838f, 0.001618f, 0.940506f, 1.0f,
	-0.525360f, 0.001617f, -0.797737f, 3.0f,
	0.312387f, 1.754014f, -1.274482f, 2.0f,
	//
	-1.412154f, 1.716662f, 0.477488f, 0.0f,
	-0.532825f, -0.002572f, 0.998076f, 1.0f,
	-0.393268f, 1.716662f, -1.243520f, 2.0f,
	-0.532825f, -0.002572f, 0.998076f, 1.0f,
	0.486062f, -0.002572f, -0.722932f, 3.0f,
	-0.393268f, 1.716662f, -1.243520f, 2.0f,
};

static constexpr const unsigned int gVertexBuffer_Bush_Size = (sizeof(gVertexBuffer_Bush) / sizeof(gVertexBuffer_Bush[0])) / 4;

const std::string& Plant::SERIALIZE_ID = "Plant";

//-----------------------------------------------------------------------------

Plant::Plant(std::weak_ptr<Planet> planet):
	mPlanet(planet),
	mPlantMode(false)
{
	mPin = std::make_unique<Pin>();

	mTextures[0] = std::make_unique<Texture>(1, IoUtils::resource("/texture/plant/plant1.png"));
	mTextures[0]->mipmap()->bind();

	mTextures[1] = std::make_unique<Texture>(2, IoUtils::resource("/texture/plant/plant2.png"));
	mTextures[1]->mipmap()->bind();

	mTextures[2] = std::make_unique<Texture>(3, IoUtils::resource("/texture/plant/plant3.png"));
	mTextures[2]->mipmap()->bind();

	mTextures[3] = std::make_unique<Texture>(4, IoUtils::resource("/texture/plant/plant4.png"));
	mTextures[3]->mipmap()->bind();

	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "vertex");
	mShader->bindAttribute(1, "position");
	mShader->bindAttribute(2, "info");
	mShader->link();

	mShadowShader = std::make_unique<Shader>(vs, fsShadow);
	mShadowShader->bindAttribute(0, "vertex");
	mShadowShader->bindAttribute(1, "position");
	mShadowShader->bindAttribute(2, "info");
	mShadowShader->link();

	// The VBO containing the shared vertices.
	// Thanks to instancing, they will be shared by all instances.
	glGenBuffers(1, &mVbo);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVertexBuffer_Bush), &gVertexBuffer_Bush[0], GL_STATIC_DRAW);
}


Plant::~Plant() {
	glDeleteBuffers(1, &mVbo);

	std::for_each(mPagePoints.begin(), mPagePoints.end(), [](auto& iterator){
		iterator.second.deleteBuffers();
	});
	mPagePoints.clear();
}


ISceneObject::CommandMap Plant::getCommands() {
	ISceneObject::CommandMap map;

	map["plant"] = [this](const std::string& param) {
		mPlantMode = true;
		Log::debug("Plant mode on");
	};

	return map;
}

const static btVector3 gY(0.f, 1.0, 0.f);
constexpr const static float _2PI = 6.28318530717959f;

void Plant::addPlants(const btVector3& point) {
	const btVector3& up = point.normalized();
	const btVector3& left = gY.cross(up).normalized();

	std::shared_ptr<Planet> planet = mPlanet.lock();

	float brushSize = planet->getBrushSize();
	std::vector<unsigned int> modifiedPageIds;
	modifiedPageIds.reserve(10);

	static const auto randomValue = [] {
		return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	};

	for (int i = 0; i < brushSize; i++) {
		const btVector3& direction = left.rotate(up, randomValue() * _2PI);
		float random2 = randomValue();
		float length = -4.f * random2 * random2 + 4.f * random2; // parabola (upside down / peak at x = 0.5)
		const btVector3& shift = length * brushSize * direction;

		PlantData data;
		data.position = planet->getSurfacePoint(point + shift);
		data.info = {
			1.0f + randomValue() * 2.0f, // side
			_2PI * randomValue(), // rotation
			4.0f * randomValue() // 0.0 .. 4.0 ==> four textures
		};

		unsigned int pageId = planet->getPageId(data.position);
		mPagePoints[pageId].points.push_back(data);
		modifiedPageIds.push_back(pageId);

		Log::debug("Added plant | page ID %d | %.2f, %.2f, %.2f", pageId, data.position.x(), data.position.y(), data.position.z());
	}
	for (auto&& pageId : modifiedPageIds) {
		mPagePoints[pageId].bind();
	}
}


void Plant::removePlants(const btVector3& mouse3d) {
	Log::debug("Removing plants around %.2f, %.2f, %.2f", mouse3d.x(), mouse3d.y(), mouse3d.z());

	std::shared_ptr<Planet> planet = mPlanet.lock();
	auto brushSize = planet.get()->getBrushSize();

	for (auto& iterator : mPagePoints) {
		std::vector<PlantData>& points = iterator.second.points;
		std::vector<PlantData> toBeRemoved;
		toBeRemoved.reserve(points.size());
		for (PlantData& data : points) {
			if (data.position.distance(mouse3d) <= brushSize)
				toBeRemoved.push_back(data);
		}
		std::vector<PlantData> clean;
		clean.reserve(points.size() - toBeRemoved.size());
		for (PlantData& data : points) {
			auto found = std::find(toBeRemoved.cbegin(), toBeRemoved.cend(), data);
			if (found == toBeRemoved.cend()) {
				clean.push_back(data);
			}
		}
		points.swap(clean);
		iterator.second.bind();
	}
}


void Plant::renderOpaque(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) {
	if (pageIds.empty())
		return;
	else if (gameState.mode == GameMode::EDITING) {
		if (mPlantMode) {
			if (gameState.isLeftMouseDown) {
				addPlants(gameState.mouse3d);
				mPlantMode = false;
			} else if (gameState.isRightMouseDown) {
				removePlants(gameState.mouse3d);
				mPlantMode = false;
			}
		}

		// Show pins
		static const btVector3 PIN_COLOR(0.8f, 0.8f, 0.8f);
		for (auto& pIt : pageIds) {
			auto iterator = mPagePoints.find(pIt.first);
			if (iterator != mPagePoints.end()) {
				static std::vector<btVector3> points;
				points.clear();
				for (unsigned long i = 0; i < iterator->second.points.size(); ++i)
					points.emplace_back(iterator->second.points[i].position);

				mPin->render(points, camera, sky->getSunPosition(), PIN_COLOR);
			}
		}
	} else if (shadowMap->isRendering()) {
		glDisable(GL_CULL_FACE);
		render(pageIds, camera, sky, shadowMap);
		glEnable(GL_CULL_FACE);
	}
}


void Plant::renderTranslucent(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) {
	if (gameState.mode != GameMode::EDITING && !pageIds.empty()) {
		render(pageIds, camera, sky, shadowMap);
	}
}


void Plant::render(const std::unordered_map<unsigned int,float>& pageIds, const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap) {
	bool isShadowRendering = shadowMap->isRendering();
	IShader* pShader = isShadowRendering? mShadowShader.get() : mShader.get();

	pShader->run();
	sky->setVars(pShader, camera);
	ShaderNoise::setVars(pShader);

	if (isShadowRendering)
		shadowMap->setMatrices(pShader);
	else {
		camera->setMatrices(pShader);
		shadowMap->setVars(pShader);
	}

	pShader->set("texture0", mTextures[0].get());
	pShader->set("texture1", mTextures[1].get());
	pShader->set("texture2", mTextures[2].get());
	pShader->set("texture3", mTextures[3].get());

	// shared vertices
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glVertexAttribDivisor(0, 0); // vertices : always reuse the same vertices	-> 0
	glVertexAttribDivisor(1, 1); // positions : one per quad (its center)		-> 1
	glVertexAttribDivisor(2, 1); // info : one per quad 						-> 1

	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	for (auto& pIt : pageIds) {
		auto it = mPagePoints.find(pIt.first);
		if (it != mPagePoints.end()) {
			glBindBuffer(GL_ARRAY_BUFFER, it->second.pbo);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PlantData), 0);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(PlantData), (GLvoid*) offsetof(PlantData, info));
			glDrawArraysInstanced(GL_TRIANGLES, 0, gVertexBuffer_Bush_Size /* N vertices*/, it->second.points.size());
		}
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glVertexAttribDivisor(1, 0); // disable
	glVertexAttribDivisor(2, 0); // disable

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	IShader::stop();
}


void Plant::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mPagePoints.size());
	for (auto& it : mPagePoints) {
		serializer->write(it.first);
		it.second.write(serializer);
	}
}


std::pair<std::string, Factory> Plant::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(window->getGameScene()->getSerializable(Planet::SERIALIZE_ID));
		std::shared_ptr<Plant> o = std::make_shared<Plant>(planet);
		o->setObjectId(objectId);

		PlantPageMap::size_type mapSize;
		serializer->read(mapSize);

		unsigned int pageId;
		for (PlantPageMap::size_type i = 0; i < mapSize; ++i) {
			serializer->read(pageId);
			o->mPagePoints[pageId] = PlantPageData::read(serializer);
			o->mPagePoints[pageId].bind();
		}

		window->getGameScene()->addSerializable(o);
		window->getGameScene()->addCommands(o->getCommands());
		planet->addExternalObject(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& Plant::serializeID() const noexcept {
	return SERIALIZE_ID;
}










