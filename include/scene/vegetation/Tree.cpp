#include "Tree.h"
#include "../../util/IoUtils.h"
#include "../sky/SkyShader.h"
#include "../../util/ShadowMap.h"
#include "../../util/Texture.h"
#include "../../util/ShaderNoise.h"
#include "../../util/ShaderUtils.h"


static constexpr const char* vs[] = {
	"attribute vec3 vertex;"
	"attribute vec2 uv;"
	"attribute vec3 position;"
	"attribute vec3 info;"

	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 cameraPosition;"
	"uniform bool hasWind;"

	"varying vec3 worldV;"
	"varying vec2 _uv;"
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

	"const vec3 DOWN1 = vec3(0.0, -1.0, 0.0);"

	"void main() {"
		"_uv = uv;"

		"_distance = distance(cameraPosition, position);"
		"vec3 noise = _distance < 250.0 && hasWind? noiseNormal(vertex) + DOWN1 : vec3(0.0);"

		"worldV = position + toPlanetSurface(position, noise + rotateY(info.x * vertex, info.y));"

		"vec4 worldV_4 = vec4(worldV, 1.0);"
		"setShadowMap(worldV_4);"
		"gl_Position = P * V * worldV_4;"
	"}"
};

static constexpr const char* fs[] = {
	"varying vec3 worldV;"
	"varying vec2 _uv;"
	"varying float _distance;"

	"uniform sampler2D texture;"

	,ShadowMap::getFragmentShaderCode(),
	""
	,SkyShader::SHARED_FRAGMENT_CODE,

	"vec4 color() {"
		"return texture2D(texture, _uv);"
	"}"

	"void main() {"
		"vec4 c = color();"
		"if (c.a <= 0.2)"
			"discard;"
		"float shadow = _distance > 300.0? 0.6 : max(getShadow9(worldV), 0.4);"
		"gl_FragColor = vec4(c.xyz * shadow, c.a);"
		"gl_FragColor = sky_mixHaze(gl_FragColor, _distance, worldV);"
	"}"
};


static constexpr const char* fsShadow[] = {
	"varying vec3 worldV;"
	"varying vec4 eyeV;"
	"varying vec2 _uv;"

	"uniform sampler2D texture;"

	"vec4 color() {"
		"return texture2D(texture, _uv);"
	"}"

	"void main() {"
		"vec4 c = color();"
		"if (c.a < 0.25)"
			"discard;"
		"gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);"
	"}"
};


const std::string& Tree::SERIALIZE_ID = "Tree";

//-----------------------------------------------------------------------------

Tree::Tree(std::weak_ptr<Planet> planet):
	mPlanet(planet)
{
	mPin = std::make_unique<Pin>();

	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "vertex");
	mShader->bindAttribute(1, "uv");
	mShader->bindAttribute(2, "position");
	mShader->bindAttribute(3, "info");
	mShader->link();

	mShadowShader = std::make_unique<Shader>(vs, fsShadow);
	mShadowShader->bindAttribute(0, "vertex");
	mShadowShader->bindAttribute(1, "uv");
	mShadowShader->bindAttribute(2, "position");
	mShadowShader->bindAttribute(3, "info");
	mShadowShader->link();
}


Tree::~Tree() {
	Log::debug("Deleting Tree");
}


void Tree::addModel(const std::string& name, const std::string& filename, float maxDistance, unsigned int windMesh) {
	Log::debug("Adding tree model[%s] = %s (%.2f)", name.c_str(), filename.c_str(), maxDistance);
	if (!mModelGroups[name])
		mModelGroups[name] = std::make_unique<ModelGroup>();
	mModelGroups[name]->modelLOD[maxDistance] = std::make_unique<ModelData>(filename, windMesh);
}


ISceneObject::CommandMap Tree::getCommands() {
	ISceneObject::CommandMap map;

	map["tree"] = [this](const std::string& param) {
		mTreeName = param;
		Log::debug("Tree mode on");
	};

	return map;
}

const static btVector3 gY(0.f, 1.0, 0.f);
constexpr const static float _2PI = 6.28318530717959f;

void Tree::addTrees(const btVector3& point, const std::string& treeName) {
	const btVector3& up = point.normalized();
	const btVector3& left = gY.cross(up).normalized();

	std::shared_ptr<Planet> planet = mPlanet.lock();

	float brushSize = planet->getBrushSize();
	std::vector<TreePageData*> modifiedPageData;
	modifiedPageData.reserve(20);

	static const auto randomValue = [] {
		return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
	};

	for (int i = 0; i < brushSize / 10; i++) {
		const btVector3& direction = left.rotate(up, randomValue() * _2PI);
		float random2 = randomValue();
		float length = -4.f * random2 * random2 + 4.f * random2; // parabola (upside down / peak at x = 0.5)
		const btVector3& shift = length * brushSize * direction;

		TreeData data;
		data.position = planet->getSurfacePoint(point + shift);
		data.info = {
			0.3f + randomValue(), // size
			_2PI * randomValue(), // rotation
			4.0f * randomValue() // 0.0 .. 4.0 ==> four textures
		};

		auto& modelGroup = mModelGroups[treeName];

		unsigned int pageId = planet->getPageId(data.position);
		if (modelGroup->pagePoints.find(pageId) == modelGroup->pagePoints.end()) {
			modelGroup->pagePoints[pageId] = std::make_unique<TreePageData>();
		}
		modelGroup->pagePoints[pageId]->points.push_back(data);
		modifiedPageData.push_back(modelGroup->pagePoints[pageId].get());

		Log::debug("Added Tree | page ID %d | %.2f, %.2f, %.2f", pageId, data.position.x(), data.position.y(), data.position.z());
	}
	for (auto& data : modifiedPageData) {
		data->bind();
	}
}


void Tree::removeTrees(const btVector3& point) {
	Log::debug("Removing trees around %.2f, %.2f, %.2f", point.x(), point.y(), point.z());

	std::shared_ptr<Planet> planet = mPlanet.lock();
	auto brushSize = planet.get()->getBrushSize();

	for (auto& modelGroup : mModelGroups) {
		for (auto& iterator : modelGroup.second->pagePoints) {
			std::vector<TreeData>& points = iterator.second->points;
			std::vector<TreeData> toBeRemoved;
			toBeRemoved.reserve(points.size());
			for (TreeData& data : points) {
				if (data.position.distance(point) <= brushSize)
					toBeRemoved.push_back(data);
			}
			std::vector<TreeData> clean;
			clean.reserve(points.size() - toBeRemoved.size());
			for (TreeData& data : points) {
				auto found = std::find(toBeRemoved.cbegin(), toBeRemoved.cend(), data);
				if (found == toBeRemoved.cend()) {
					clean.push_back(data);
				}
			}
			points.swap(clean);
			iterator.second->bind();
		}
	}
}


void Tree::render(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap) {
	bool hasPoints = false;
	for (auto& modelGroup : mModelGroups) {
		if (modelGroup.second->pagePoints.size() > 0) {
			hasPoints = true;
			break;
		}
	}
	if (!hasPoints)
		return;

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

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	glVertexAttribDivisor(0, 0); // vertices : always reuse the same vertices	-> 0
	glVertexAttribDivisor(1, 0); // uv : always reuse the same uv 				-> 0
	glVertexAttribDivisor(2, 1); // positions : one per quad (its center)		-> 1
	glVertexAttribDivisor(3, 1); // info : one per quad 						-> 1

	for (auto& modelGroup : mModelGroups)
		modelGroup.second->render(pageIds, pShader);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glVertexAttribDivisor(2, 0); // disable
	glVertexAttribDivisor(3, 0); // disable

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	IShader::stop();
}


void Tree::renderOpaque(const std::unordered_map<unsigned int,float>& pageIds, const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const GameState& gameState) {
	if (pageIds.empty())
		return;
	else if (gameState.mode == GameMode::EDITING) {
		if (!mTreeName.empty()) {
			if (gameState.isLeftMouseDown) {
				addTrees(gameState.mouse3d, mTreeName);
				mTreeName = "";
			} else if (gameState.isRightMouseDown) {
				removeTrees(gameState.mouse3d);
				mTreeName = "";
			}
		}

		// Show pins
		static const btVector3 PIN_COLOR(0.4f, 0.2f, 0.8f);
		for (auto& pIt : pageIds) {
			for (auto& modelGroup : mModelGroups) {
				auto& pagePoints = modelGroup.second->pagePoints;
				auto iterator = pagePoints.find(pIt.first);
				if (iterator != pagePoints.end()) {
					static std::vector<btVector3> points;
					points.clear();
					for (unsigned long i = 0; i < iterator->second->points.size(); ++i)
						points.emplace_back(iterator->second->points[i].position);

					mPin->render(points, camera, sky->getSunPosition(), PIN_COLOR);
				}
			}
		}
	} else {
		render(pageIds, camera, sky, shadowMap);
	}
}


void Tree::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mModelGroups.size());
	for (auto& m : mModelGroups) {
		serializer->write(m.first);
		m.second->write(serializer);
	}
}


std::pair<std::string, Factory> Tree::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(window->getGameScene()->getSerializable(Planet::SERIALIZE_ID));
		std::shared_ptr<Tree> o = std::make_shared<Tree>(planet);
		o->setObjectId(objectId);

		unsigned long count;
		serializer->read(count);

		std::string name;
		for (unsigned long c = 0; c < count; ++c) {
			serializer->read(name);
			o->mModelGroups[name] = ModelGroup::read(serializer);
		}
		window->getGameScene()->addSerializable(o);
		window->getGameScene()->addCommands(o->getCommands());
		planet->addExternalObject(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& Tree::serializeID() const noexcept {
	return SERIALIZE_ID;
}

//-----------------------------------------------------------------------------

void Tree::ModelGroup::render(const std::unordered_map<unsigned int,float>& pageIds, IShader* shader) {
	static std::vector<TreePageData*> pageData(40);

	static const auto findTreePageData = [](float min, float max, const auto& pageIds, const auto& pagePoints){
		for (auto& pIt : pageIds) {
			unsigned int pageId = pIt.first;
			float d = pIt.second;
			if (d > min && d <= max) {
				const auto& pp = pagePoints.find(pageId);
				if (pp != pagePoints.end()) {
					pageData.push_back(pp->second.get());
				}
			}
		}
	};

	float minLimit = 0.0f;
	// For each level of detail
	for (auto& lod : modelLOD) {
		pageData.clear();
		// Now collect all TreePageData within the current LOD limits
		float maxLimit = lod.first;
		findTreePageData(minLimit, maxLimit, pageIds, pagePoints);
		// Render
		lod.second->render(pageData, shader);
		// Move the limit up
		minLimit = maxLimit;
	}
}


void Tree::ModelGroup::write(ISerializer *serializer) const {
	serializer->write(modelLOD.size());
	for (auto& m : modelLOD) {
		serializer->write(m.first);
		m.second->write(serializer);
	}
	serializer->write(pagePoints.size());
	for (auto& pp : pagePoints) {
		serializer->write(pp.first);
		pp.second->write(serializer);
	}
}


std::unique_ptr<Tree::ModelGroup> Tree::ModelGroup::read(ISerializer *serializer) {
	std::unique_ptr<ModelGroup> o = std::make_unique<ModelGroup>();

	unsigned long count;
	serializer->read(count);
	float distance;
	for (unsigned long c = 0; c < count; ++c) {
		serializer->read(distance);
		o->modelLOD[distance] = ModelData::read(serializer);
	}

	serializer->read(count);
	unsigned int pageId;
	for (unsigned long c = 0; c < count; ++c) {
		serializer->read(pageId);
		o->pagePoints[pageId] = TreePageData::read(serializer);
		o->pagePoints[pageId]->bind();
	}
	return o;
}

//-----------------------------------------------------------------------------

Tree::ModelData::ModelData(const std::string& filename, unsigned int windMesh):
	filename(filename),
	windMesh(windMesh)
{
	auto fullpath = IoUtils::resource(filename);
	modelOBJ = std::make_unique<ModelOBJ>();
	if (!modelOBJ->import(fullpath.c_str())) {
		Log::error("Failed to load tree model: %s", fullpath.c_str());
		exit(-1);
	}

	// The VBO containing the shared vertices. Thanks to instancing, they will be shared by all instances.
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, modelOBJ->getNumberOfVertices() * modelOBJ->getVertexSize(), &modelOBJ->getVertexBuffer()->position[0], GL_STATIC_DRAW);

	const auto bindMesh = [](GLuint ibo, const ModelOBJ* model, unsigned int meshIndex){
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					 3 * model->getMesh(meshIndex).triangleCount * model->getIndexSize(),
					 model->getIndexBuffer() + model->getMesh(meshIndex).startIndex,
					 GL_STATIC_DRAW);
	};

	meshes.reserve(modelOBJ->getNumberOfMeshes());
	for (unsigned int i = 0; i < modelOBJ->getNumberOfMeshes(); ++i) {
		auto mesh = std::make_unique<MeshData>();

		glGenBuffers(1, &mesh->ibo);
		bindMesh(mesh->ibo, modelOBJ.get(), i);

		mesh->indexCount = 3 * modelOBJ->getMesh(i).triangleCount;
		mesh->hasWind = i == windMesh;
		mesh->texture = std::make_unique<Texture>(1, IoUtils::resource("/model/vegetation/" + modelOBJ->getMesh(i).pMaterial->colorMapFilename));
		mesh->texture->mipmap()->bind();

		meshes.push_back(std::move(mesh));
	}

	// Unbind
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


Tree::ModelData::~ModelData() {
	glDeleteBuffers(1, &vbo);
}


void Tree::ModelData::render(const std::vector<TreePageData*>& pageData, IShader* shader) {
	// shared vertices
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, modelOBJ->getVertexSize(), 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, modelOBJ->getVertexSize(), (GLvoid*) offsetof(ModelOBJ::Vertex, texCoord));

	static const auto renderMesh = [](const MeshData* mesh, IShader* pShader, const std::vector<TreePageData*>& pageData) {
		pShader->set("texture", mesh->texture.get());
		// shared indices
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibo);

		for (auto& data : pageData) {
			glBindBuffer(GL_ARRAY_BUFFER, data->pbo);
			glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(TreeData), 0);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(TreeData), (GLvoid*) offsetof(TreeData, info));
			glDrawElementsInstanced(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, 0, data->points.size());
		}
	};

	glDisable(GL_CULL_FACE);

	for (const auto& mesh : meshes) {
		shader->set("hasWind", mesh->hasWind);
		renderMesh(mesh.get(), shader, pageData);
	}

	glEnable(GL_CULL_FACE);
}


void Tree::ModelData::write(ISerializer *serializer) const {
	serializer->write(filename);
	serializer->write(windMesh);
}


std::unique_ptr<Tree::ModelData> Tree::ModelData::read(ISerializer *serializer) {
	std::string filename;
	serializer->read(filename);

	unsigned int windMesh;
	serializer->read(windMesh);

	std::unique_ptr<Tree::ModelData> o = std::make_unique<Tree::ModelData>(filename, windMesh);
	return o;
}





