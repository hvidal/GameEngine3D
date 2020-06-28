#include <chrono>
#include "Planet.h"
#include "../../util/Shader.h"
#include "../../util/ShadowMap.h"
#include "../../util/Texture.h"
#include "SurfaceReflection.h"
#include "../../util/ShaderUtils.h"
#include "../../util/ShaderNoise.h"
#include "../sky/SkyShader.h"


static const char* _vs[] = {
	"attribute vec4 position;"
	"attribute vec4 normal;"
	"attribute vec4 material;"
	"attribute vec2 uv;"

	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 sunPosition;"

	"varying vec3 worldV;"
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec3 _uv;"
	"varying vec4 _material;"

	,ShadowMap::getVertexShaderCode(),

	"uniform vec3 cameraPosition;"
	,SurfaceReflection::getVertexShaderCode(),
	" "
	,ShaderUtils::TO_MAT3,

	"void main() {"
		"setShadowMap(position);"

		"vec3 position0 = planetSurfaceReflection(position.xyz);"
		"_uv = vec3(uv.xy, waterLevel - length(position.xyz));"
		"_material = material;"
		"worldV = position.xyz;"

		"eyeV = V * position;"
		"eyeN = toMat3(V) * normal.xyz;"
		"eyeL = V * vec4(sunPosition, 1.0);"
		"gl_Position = P * V * vec4(position0, 1.0);"
	"}"
};

static constexpr const char* _fs[] = {
	"uniform sampler2D material0;"
	"uniform float material0_scale;"

	"uniform sampler2D material1;"
	"uniform float material1_scale;"

	"uniform sampler2D material2;"
	"uniform float material2_scale;"

	"uniform sampler2D material3;"
	"uniform float material3_scale;"

	,SkyShader::SHARED_FRAGMENT_CODE,
	" "
	,ShadowMap::getFragmentShaderCode(),

	"uniform vec3 mousePosition;"
	"uniform float brushSize;"

	"varying vec3 worldV;"
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec4 _material;"
	"varying vec3 _uv;"

	// PlanetSurfaceReflection
	"uniform bool isRenderingReflection;"
	"uniform float waterLevel;"
	"uniform float distanceToHorizon;"

	"const float MAX_DEPTH = 30.0;"
	"const vec4 WATER_SHALLOW_COLOR = vec4(124, 183, 160, 255) / 255.0;"
	"const vec4 WATER_DEEP_COLOR = vec4(51, 95, 92, 255) / 255.0;"

	"vec4 color(vec3 eyeV, vec3 eyeN, vec3 eyeL) {"
		"vec4 matColor = vec4(0.0);"
		"float shadow = 1.0;"
		"float depth = _uv.z;"
		"depth = clamp(depth, 0.0, MAX_DEPTH) / MAX_DEPTH;"
		"if (depth > 0.9999) {"
			"matColor = WATER_DEEP_COLOR;"
		"} else {"
			// start combining the four materials into a single color
			"vec4 c0 = texture2D(material0, _uv.xy * material0_scale);"
			"matColor = mix(matColor, c0, _material.x);"

			"vec4 c1 = texture2D(material1, _uv.xy * material1_scale);"
			"matColor = mix(matColor, c1, _material.y);"

			"vec4 c2 = texture2D(material2, _uv.xy * material2_scale);"
			"matColor = mix(matColor, c2, _material.z);"

			"vec4 c3 = texture2D(material3, _uv.xy * material3_scale);"
			"matColor = mix(matColor, c3, _material.a);"
			// - end combining

			"if (depth > 0.0) {"
				"vec4 waterColor = mix(WATER_SHALLOW_COLOR, WATER_DEEP_COLOR, depth);"
				"matColor = mix(matColor, waterColor, depth);"
			"}"

			"if (!isRenderingReflection)"
				"shadow = getShadow9(worldV);"
		"}"
		"return sky_lighting(matColor, eyeV, eyeN, eyeL, shadow);"
	"}"

	"void main() {"
		"if (isRenderingReflection) {"
			// if the point is under water, discard it
			"float height = length(worldV);"
			"if (height <= waterLevel)"
				"discard;"
			// If the point is beyond the horizon and behind the planet, discard it
			"if (-eyeV.z > distanceToHorizon) {"
				"float dotOriginToPoint = sky_dotOriginToPoint(worldV.xyz);"
				"if (dotOriginToPoint > dotOriginToHorizon)"
					"discard;"
			"}"
		"}"

		"gl_FragColor = color(eyeV.xyz, eyeN, eyeL.xyz);"

		"if (!isRenderingReflection) {"
			"float d = distance(mousePosition, worldV);"
			"if (d <= brushSize)"
				"gl_FragColor *= vec4(0.8, 0.8, 0.8, 0.2);"

			// apply haze
			"gl_FragColor = sky_mixHaze(gl_FragColor, -eyeV.z, worldV);"
		"}"
		"gl_FragColor.a = 1.0;"
	"}"
};

static constexpr const char* _vsWater[] = {
	"attribute vec4 position;"
	"attribute vec2 uv;"

	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform float waterLevel;"
	"uniform vec3 sunPosition;"

	"varying vec3 worldV;"
	"varying vec4 projectedV;"
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec3 _uv;"

	,ShaderNoise::SHADER_CODE,

	"const float noiseScale = 250.0;"

	,ShaderUtils::TO_MAT3,

	"void main() {"
		"vec3 p = position.xyz;"
		"_uv = vec3(uv.xy * noiseScale, waterLevel - length(p));"
		// Use displacement to move vertices up and down
		"vec3 up1 = normalize(p);"
		"float noise = -7.0 + 5.0 * noiseAt(20.0 * vec3(uv.x, 0.0, uv.y));"
		"worldV = (waterLevel + noise) * up1;"
		"vec4 p4 = vec4(worldV, 1.0);"
		"eyeV = V * p4;"
		"eyeN = toMat3(V) * up1;"
		"eyeL = V * vec4(sunPosition, 1.0);"
		"projectedV = P * V * p4;"
		"gl_Position = projectedV;"
	"}"
};

static constexpr const char* _fsWater[] = {
	"varying vec3 worldV;"
	"varying vec4 projectedV;"
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec3 _uv;"

	"uniform sampler2D reflectionTexture;"

	"uniform sampler2D waterNormalTexture;"

	,SkyShader::SHARED_FRAGMENT_CODE,
	" "
	,ShaderNoise::SHADER_CODE,

	"const float MAX_DEPTH = 20.0;"
	"const vec3 WATER_SHALLOW_COLOR = vec3(124, 183, 160) / 255.0;"
	"const vec3 WATER_DEEP_COLOR = vec3(49, 78, 76) / 255.0;"

	,ShaderUtils::TO_PLANET_SURFACE,

	"vec2 texCoord(vec2 shift) {"
		"vec2 c = (vec2(projectedV) + shift) / projectedV.w;"
		"c = (c + 1.0) * 0.5;"
		"return clamp(c, 0.0, 1.0);"
	"}"

	"void main() {"
		// if above water level, then discard
		"if (_uv.z < 0.0)"
			"discard;"

		"vec3 vCameraToPoint1 = normalize(worldV - cameraPosition);"
		"vec3 vUp1 = normalize(worldV);"
		// if water is beyond the horizon, discard
		"if (dot(vCameraToPoint1, vUp1) > 0.0)"
			"discard;"

		"vec3 noiseNormalA = noiseNormal(vec3(_uv.x, 0.0, _uv.y));"
		"vec3 noiseNormal1 = toPlanetSurface(worldV, noiseNormalA);" // convert to world space
		"float reflection = 0.2 * dot(noiseNormal1, vUp1);"

		"float depth = clamp(_uv.z, 0.0, MAX_DEPTH) / MAX_DEPTH;"
		"vec3 waterColor = mix(WATER_SHALLOW_COLOR, WATER_DEEP_COLOR, depth);"
		"vec3 reflectedColor = texture2D(reflectionTexture, texCoord(10.0 * noiseNormalA.xz)).xyz;"

		"vec3 c = 0.7 * mix(waterColor, reflectedColor, reflection);"

		// Sun specular
		"vec3 vSunToPoint1 = normalize(worldV - sunPosition);"
		"vec3 vReflectedLight1 = reflect(vSunToPoint1, noiseNormal1);"
		"float pSun = dot(vReflectedLight1, -vCameraToPoint1);"
		"if (pSun > 0.0) {"
			"float dayLight = clamp(dot(vUp1, -vSunToPoint1), 0.1, 1.0);" // 0.1 (night side) .. 1.0 (noon)
			"float specularA = pSun <= 0.0? 0.0 : dayLight * 0.25 * pow(pSun, 30.0);"
			"float specularB = pSun <= 0.0? 0.0 : dayLight * 0.05 * abs(pSun);"
			"c = c + specularA + specularB;"
		"}"

		"gl_FragColor = vec4(c, 1.0);"
		"gl_FragColor = sky_lighting(gl_FragColor, eyeV.xyz, eyeN, eyeL.xyz, 1.0);"
		"gl_FragColor = sky_mixHaze(gl_FragColor, -eyeV.z, worldV);"

		// alpha depends on the depth of the water
		// when the water touches the terrain, alpha should be zero
		"float alpha = clamp(depth, 0.0, 0.5) / 0.5;"
		"gl_FragColor.a = alpha * alpha;"
	"}"
};


const std::string& Planet::SERIALIZE_ID = "Planet";

//-----------------------------------------------------------------------------

Planet::Planet(std::shared_ptr<btDynamicsWorld> dynamicsWorld, float radius, float waterLevel):
	SceneObject(dynamicsWorld),
	mRadius(radius),
	mWaterLevel(waterLevel),
	mBrushSize(50.f)
{}


Planet::Planet(std::shared_ptr<btDynamicsWorld> dynamicsWorld, float radius, float waterLevel, unsigned int faceDivisions, unsigned int pageDivisions):
	Planet(dynamicsWorld, radius, waterLevel)
{
	mFaces[0] = std::make_unique<PlanetFace>(1000, "+zx", radius, waterLevel, faceDivisions, pageDivisions);
	mFaces[1] = std::make_unique<PlanetFace>(2000, "-zx", radius, waterLevel, faceDivisions, pageDivisions);
	mFaces[2] = std::make_unique<PlanetFace>(3000, "+xy", radius, waterLevel, faceDivisions, pageDivisions);
	mFaces[3] = std::make_unique<PlanetFace>(4000, "-xy", radius, waterLevel, faceDivisions, pageDivisions);
	mFaces[4] = std::make_unique<PlanetFace>(5000, "+yz", radius, waterLevel, faceDivisions, pageDivisions);
	mFaces[5] = std::make_unique<PlanetFace>(6000, "-yz", radius, waterLevel, faceDivisions, pageDivisions);
}


Planet::~Planet() {
	Log::debug("Deleting planet");
}


constexpr const static float HEIGHT_INCREMENT = 0.05f;

btVector3 Planet::getSurfacePoint(btVector3 p, float extraIncrement) const {
	float height = getHeightAt(p) + HEIGHT_INCREMENT + extraIncrement;
	return height * p.normalized();
}


btTransform Planet::getSurfaceTransform(btVector3 p, float extraIncrement) const {
	btVector3 up = p.normalized();
	btVector3 left = btVector3(0.0f, 1.0f, 0.0f).cross(up);
	btVector3 front = up.cross(left);
	btVector3 origin = getSurfacePoint(p, extraIncrement);
	return btTransform(btMatrix3x3(front, up, left), origin);
}


void Planet::moveTo(Matrix4x4& matrix, const btVector3& direction, float speed) {
	const btVector3& origin = matrix.getOrigin();
	btVector3 up1 = origin.normalized();
	btVector3 left1 = up1.cross(direction).normalized();
	btVector3 front1 = up1.cross(left1).normalized();
	// move the origin
	btVector3 newOrigin = getSurfacePoint(origin + speed * front1);
	up1 = newOrigin.normalized();
	front1 = up1.cross(left1).normalized();
	matrix.setRow(0, front1);
	matrix.setRow(1, up1);
	matrix.setRow(2, left1);
	matrix.setOrigin(newOrigin);
}


void Planet::initPhysics(btTransform transform) {
	mShader = std::make_unique<Shader>(_vs, _fs);
	mShader->bindAttribute(0, "position");
	mShader->bindAttribute(1, "normal");
	mShader->bindAttribute(2, "material");
	mShader->bindAttribute(3, "uv");
	mShader->link();

	mWaterShader = std::make_unique<Shader>(_vsWater, _fsWater);
	mWaterShader->bindAttribute(0, "position");
	mWaterShader->bindAttribute(1, "uv");
	mWaterShader->link();

	std::shared_ptr<btDynamicsWorld> dynamicsWorld = mDynamicsWorld.lock();
	btDynamicsWorld* pDynamicsWorld = dynamicsWorld.get();
	size_t pageCount = 0;
	for (auto& face : mFaces) {
		face->initPhysics(pDynamicsWorld);
		pageCount += face->getPageCount();
	}

	mVisiblePages.reserve(pageCount);
}


void Planet::updateSimulation() {
	std::shared_ptr<btDynamicsWorld> dynamicsWorld = mDynamicsWorld.lock();
	btDynamicsWorld* pDynamicsWorld = dynamicsWorld.get();
	for (auto& face : mFaces) {
		face->enablePhysics(pDynamicsWorld, mInteractiveBodies);
	}
}


void Planet::setTexture(unsigned int index, std::unique_ptr<ITexture>&& colorTexture) {
	if (index >= mTextureArray.size())
		throw std::runtime_error("Invalid texture index: " + std::to_string(index));
	mTextureArray[index] = std::move(colorTexture);
}


float Planet::getHeightAt(const btVector3& direction) const {
	for (const auto& face : mFaces) {
		float h = face->getHeightAt(direction);
		if (h > 0.0f)
			return h;
	}
	Log::error("Height NOT found for (%.2f, %.2f, %.2f)", direction.x(), direction.y(), direction.z());
	throw std::runtime_error("Height should have been found.");
}

std::string		gParameterValue;
std::string		gCurrentAction;
float gFactor = 5.f;


void Planet::runAction(const GameState& gameState, const ICamera* camera) {
	const btVector3& innerPoint = PlanetPage::getInnerPoint(camera, mRadius);
	if (gCurrentAction == "terrain") {
		const float factor = gameState.isLeftMouseDown? gFactor : -gFactor;
		for (auto& face : mFaces) {
			face->editVertices("terrain", factor, gameState.mouse3d, mBrushSize, camera, innerPoint);
		}
	} else if (gCurrentAction == "mat0" || gCurrentAction == "mat1" || gCurrentAction == "mat2" || gCurrentAction == "mat3") {
		for (auto& face : mFaces) {
			face->editVertices(gCurrentAction, (float) ::atof(gParameterValue.c_str()), gameState.mouse3d, mBrushSize, camera, innerPoint);
		}
	} else if (gCurrentAction == "autopaint") {
		for (auto& face : mFaces) {
			face->autoPaintVertices(gParameterValue, gameState.mouse3d, mBrushSize, camera, innerPoint);
		}
	} else if (gCurrentAction == "fixnormals") {
		fixBorderNormals(gameState.mouse3d, mBrushSize, camera, innerPoint);
	} else if (gCurrentAction == "height") {
		Log::debug("height = %.8f", getHeightAt(gameState.mouse3d));
	}
}


void Planet::collectVisiblePageIds(const ICamera* camera) {
	mVisiblePages.clear();
	mHasVisibleWater = false;
	const btVector3& innerPoint = PlanetPage::getInnerPoint(camera, mRadius);
	for (auto& face : mFaces) {
		face->getVisiblePageIds(camera, innerPoint, mVisiblePages, mHasVisibleWater);
	}
}


void Planet::renderOpaque(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) {
	// No need to paint anything if the shadowmap is being created
	// The terrain doesn't cast shadows
	if (shadowMap->isRendering()) {
		collectVisiblePageIds(camera);
		for (auto& o : mExternalObjects) {
			o->renderOpaque(mVisiblePages, camera, sky, shadowMap, gameState);
		}
		return;
	}

	IShader* pShader = mShader.get();

	mShader->run();
	camera->setMatrices(pShader);
	sky->setVars(pShader, camera);
	shadowMap->setVars(pShader);
	surfaceReflection->setVars(pShader, camera);
	mShader->set("brushSize", mBrushSize);

	// set materials
	for (int i = 0; i < mTextureArray.size(); i++) {
		std::string mat = "material" + std::to_string(i);
		ITexture* tex = mTextureArray[i].get();
		mShader->set(mat, tex);
		mShader->set(mat + "_scale", tex->getScaleFactor());
	}

	if (gameState.mode == GameMode::EDITING) {
		mShader->set("mousePosition", gameState.mouse3d);
		if (gameState.isLeftMouseDown || gameState.isRightMouseDown)
			runAction(gameState, camera);
	} else {
		const static btVector3 farAway(-BT_LARGE_FLOAT, -BT_LARGE_FLOAT, -BT_LARGE_FLOAT);
		mShader->set("mousePosition", farAway);
	}

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);

	for (auto& face : mFaces)
		face->renderOpaque(mVisiblePages, camera, gameState);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	IShader::stop();
	ITexture::unbind();

	for (auto& o : mExternalObjects) {
		o->renderOpaque(mVisiblePages, camera, sky, shadowMap, gameState);
	}
}


void Planet::renderTranslucent(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) {
	if (shadowMap->isRendering()) {
		for (auto& o : mExternalObjects) {
			o->renderTranslucent(mVisiblePages, camera, sky, shadowMap, gameState);
		}
		return;
	} else if (surfaceReflection->isRendering())
		return;

	if (mHasVisibleWater) {
		mWaterShader->run();
		mWaterShader->set("waterLevel", mWaterLevel);
		mWaterShader->set("sunPosition", sky->getSunPosition());

		IShader* pWaterShader = mWaterShader.get();
		camera->setMatrices(pWaterShader);
		sky->setVars(pWaterShader, camera);
		ShaderNoise::setVars(pWaterShader);
		surfaceReflection->setReflectionTexture(pWaterShader);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

		for (auto& face : mFaces)
			face->renderTranslucent(mVisiblePages, camera, gameState);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		IShader::stop();
	}

	for (auto& o : mExternalObjects) {
		o->renderTranslucent(mVisiblePages, camera, sky, shadowMap, gameState);
	}
}


ISceneObject::CommandMap Planet::getCommands() {
	ISceneObject::CommandMap map;

	map["brush"] = [this](const std::string& param) {
		mBrushSize = (float) ::atof(param.c_str());
		Log::debug("BrushSize set to %.2f", mBrushSize);
	};

	map["terrain"] = [](const std::string& param) {
		if (param == "off")
			gCurrentAction = "";
		else {
			gCurrentAction = "terrain";
			gFactor = param.length() == 0? 5.f : std::stof(param);
		}
		Log::debug("Edit terrain");
	};

	map["mat0"] = [](const std::string& param) {
		gCurrentAction = "mat0";
		gParameterValue = param;
		Log::debug("Material0 = %s", param.c_str());
	};

	map["mat1"] = [](const std::string& param) {
		gCurrentAction = "mat1";
		gParameterValue = param;
		Log::debug("Material1 = %s", param.c_str());
	};

	map["mat2"] = [](const std::string& param) {
		gCurrentAction = "mat2";
		gParameterValue = param;
		Log::debug("Material2 = %s", param.c_str());
	};

	map["mat3"] = [](const std::string& param) {
		gCurrentAction = "mat3";
		gParameterValue = param;
		Log::debug("Material3 = %s", param.c_str());
	};

	map["height"] = [](const std::string& param) {
		gCurrentAction = "height";
	};

	map["autopaint"] = [](const std::string& param) {
		Log::debug("Autopaint ON (click on the landscape to apply)");
		gCurrentAction = "autopaint";
		gParameterValue = param;
	};

	map["fixnormals"] = [this](const std::string& param) {
		Log::debug("FixNormals ON (click on the landscape to apply)");
		gCurrentAction = "fixnormals";
	};

	map["visiblepages"] = [this](const std::string& param) {
		Log::debug("Visible pages = %u", mVisiblePages.size());
		for (auto& it : mVisiblePages) {
			Log::debug("page[%u] = %.2f", it.first, it.second);
		}
		Log::debug("-------");
	};

	return map;
}


void Planet::convertPointToUV(const btVector3 &point, float *uv) {
	static constexpr const float PI = 3.14159265358f;
	const static btVector3& X = btVector3(1.f, 0.f, 0.f);
	const static btVector3& NEGY = btVector3(0.f, -1.f, 0.f);

	float angleV = point.normalized().angle(NEGY); // point on the YZ plane
	float angleH = btVector3(point.x(), 0.f, point.z()).normalized().angle(X); // point on the XZ plane

	// We need a value between 0.0 and 1.0
	// So all we have to do is divide the angle by PI
	uv[0] = angleV / PI;
	uv[1] = angleH / PI;
}


void Planet::fixBorderNormals(const btVector3& mouse3d, float brushSize, const ICamera *camera, const btVector3& innerPoint) {
	const std::function<void(const btVector3&,btVector3&)> getNormalAt = [this](const btVector3& point, btVector3& normal){
		for (auto& face : mFaces)
			face->getNormalAt(point, normal);
	};

	for (auto& face : mFaces)
		face->fixBorderNormals(getNormalAt, mouse3d, brushSize, camera, innerPoint);
}


unsigned int Planet::getPageId(const btVector3& point) const {
	for (auto& face : mFaces) {
		unsigned int pageId = face->getPageId(point);
		if (pageId > 0)
			return pageId;
	}
	Log::error("Unable to find page ID for %.2f %.2f %.2f", point.x(), point.y(), point.z());
	return 0;
}


void Planet::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());

	serializer->write(mRadius);
	serializer->write(mWaterLevel);

	mTextureArray[0]->write(serializer);
	mTextureArray[1]->write(serializer);
	mTextureArray[2]->write(serializer);
	mTextureArray[3]->write(serializer);

	for (const auto& face : mFaces) {
		face->write(serializer);
	}
}


std::pair<std::string, Factory> Planet::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window){
		float radius, waterLevel;
		serializer->read(radius);
		serializer->read(waterLevel);

		// calling private constructor, cannot call std::make_shared
		std::shared_ptr<Planet> o = std::shared_ptr<Planet>(new Planet(window->getGameScene()->getDynamicsWorld(), radius, waterLevel));
		o->setObjectId(objectId);

		auto textureFactory = serializer->getFactory(Texture::SERIALIZE_ID);
		std::string name;
		unsigned long oId;

		serializer->readBegin(name, oId);
		std::shared_ptr<ISerializable> m0 = textureFactory.create(objectId, serializer, window);
		o->mTextureArray[0] = std::dynamic_pointer_cast<Texture>(m0);
		o->mTextureArray[0]->mipmap()->repeat();

		serializer->readBegin(name, oId);
		std::shared_ptr<ISerializable> m1 = textureFactory.create(objectId, serializer, window);
		o->mTextureArray[1] = std::dynamic_pointer_cast<Texture>(m1);
		o->mTextureArray[1]->mipmap()->repeat();

		serializer->readBegin(name, oId);
		std::shared_ptr<ISerializable> m2 = textureFactory.create(objectId, serializer, window);
		o->mTextureArray[2] = std::dynamic_pointer_cast<Texture>(m2);
		o->mTextureArray[2]->mipmap()->repeat();

		serializer->readBegin(name, oId);
		std::shared_ptr<ISerializable> m3 = textureFactory.create(objectId, serializer, window);
		o->mTextureArray[3] = std::dynamic_pointer_cast<Texture>(m3);
		o->mTextureArray[3]->mipmap()->repeat();

		// Faces
		o->mFaces[0] = PlanetFace::create(serializer, o->mDynamicsWorld);
		o->mFaces[1] = PlanetFace::create(serializer, o->mDynamicsWorld);
		o->mFaces[2] = PlanetFace::create(serializer, o->mDynamicsWorld);
		o->mFaces[3] = PlanetFace::create(serializer, o->mDynamicsWorld);
		o->mFaces[4] = PlanetFace::create(serializer, o->mDynamicsWorld);
		o->mFaces[5] = PlanetFace::create(serializer, o->mDynamicsWorld);

		window->getGameScene()->addSceneObject(o);
		o->initPhysics(btTransform::getIdentity());
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& Planet::serializeID() const noexcept {
	return SERIALIZE_ID;
}






