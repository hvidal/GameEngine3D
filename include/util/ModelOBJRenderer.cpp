#include "ModelOBJRenderer.h"

static constexpr const char* vsShadow[] = {
	"attribute vec3 position;"

	"uniform mat4 M;"
	"uniform mat4 V;"
	"uniform mat4 P;"

	"void main() {"
		"gl_Position = P * V * M * vec4(position, 1.0);"
	"}"
};
static constexpr const char* fsShadow[] = {
	"void main() {"
		"gl_FragColor = vec4(0.0);"
	"}"
};

static constexpr const char* vsMaterial[] = {
	"attribute vec3 position;"
	"attribute vec3 normal;"

	"uniform mat4 M;"
	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 lightPosition;"

	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec4 worldV;"

	,ShadowMap::getVertexShaderCode(),
	" "
	,ShaderUtils::TO_MAT3,

	"void main() {"
		"worldV = M * vec4(position, 1.0);"
		"setShadowMap(worldV);"

		"eyeN = toMat3(V) * toMat3(M) * normal;"
		"eyeL = V * vec4(lightPosition, 1.0);"
		"eyeV = V * worldV;"
		"gl_Position = P * V * worldV;"
	"}"
};
static constexpr const char* fsMaterial[] = {
	"uniform vec4 ambientMaterial;"
	"uniform vec4 diffuseMaterial;"
	"uniform vec4 specularMaterial;"
	"uniform float shininess;"
	"uniform float alpha;"

	"uniform float ambientLight;"
	"uniform float diffuseLight;"
	"uniform float specularLight;"

	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec4 worldV;"

	,ShadowMap::getFragmentShaderCode(),

	"vec4 color(vec3 eyeV, vec3 eyeN, vec3 eyeL) {"
		"float shadow = getShadow9(worldV.xyz);"
		"vec3 n = normalize(eyeN);"
		"vec3 s = normalize(eyeL - eyeV);"
		"vec3 v = normalize(-eyeV);"
		"vec3 halfWay = normalize(v + s);"
		"vec4 Ka = ambientMaterial * ambientLight;"
		"vec4 Kd = diffuseMaterial * diffuseLight * shadow;"
		"vec4 Ks = specularMaterial * specularLight;"
		"return Ka + Kd * max(dot(s, eyeN), 0.05) + Ks * pow(max(dot(halfWay, n), 0.05), shininess);"
	"}"
	"void main() {"
		"gl_FragColor = color(eyeV.xyz, eyeN, eyeL.xyz);"
		"gl_FragColor.a = alpha;"
	"}"
};

static constexpr const char* vsColorTexture[] = {
	"attribute vec3 position;"
	"attribute vec3 normal;"
	"attribute vec2 texCoord;"

	"uniform mat4 M;"
	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 lightPosition;"

	,ShadowMap::getVertexShaderCode(),

	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec4 worldV;"
	"varying vec2 texCoord0;" // colorMap

	,ShaderUtils::TO_MAT3,

	"void main() {"
		"texCoord0 = vec2(texCoord.s, 1.0 - texCoord.t);" // flip texture upside down
		"worldV = M * vec4(position, 1.0);"
		"setShadowMap(worldV);"

		"eyeN = toMat3(V) * toMat3(M) * normal;"
		"eyeL = V * vec4(lightPosition, 1.0);"
		"eyeV = V * worldV;"
		"gl_Position = P * V * worldV;"
	"}"
};
static constexpr const char* fsColorTexture[] = {
	"uniform vec4 ambientMaterial;"
	"uniform vec4 diffuseMaterial;"
	"uniform vec4 specularMaterial;"
	"uniform float shininess;"
	"uniform float alpha;"

	"uniform float ambientLight;"
	"uniform float diffuseLight;"
	"uniform float specularLight;"

	"uniform sampler2D colorMap;"

	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"
	"varying vec4 worldV;"

	"varying vec2 texCoord0;"

	,ShadowMap::getFragmentShaderCode(),

	"vec4 color(vec3 eyeV, vec3 eyeN, vec3 eyeL) {"
		"vec4 texColor = texture2D(colorMap, texCoord0);"
		"float shadow = getShadow9(worldV.xyz);"
		"vec3 n = normalize(eyeN);"
		"vec3 s = normalize(eyeL - eyeV);"
		"vec3 v = normalize(-eyeV);"
		"vec3 halfWay = normalize(v + s);"
		"vec4 Ka = ambientMaterial * ambientLight * texColor;"
		"vec4 Kd = diffuseMaterial * diffuseLight * texColor * shadow;"
		"vec4 Ks = specularMaterial * specularLight;"
		"return Ka + Kd * max(dot(s, eyeN), 0.0) + Ks * pow(max(dot(halfWay, n), 0.0), shininess);"
	"}"
	"void main() {"
		"gl_FragColor = color(eyeV.xyz, eyeN, eyeL.xyz);"
		"gl_FragColor.a = alpha;"
	"}"
};

static constexpr const char* vsColorNormalTexture[] = {
	"attribute vec3 position;"
	"attribute vec3 normal;"
	"attribute vec2 texCoord;"
	"attribute vec4 tangent;"

	"uniform mat4 M;"
	"uniform mat4 V;"
	"uniform mat4 P;"
	"uniform vec3 lightPosition;"

	,ShadowMap::getVertexShaderCode(),

	"varying vec3 viewDir;"
	"varying vec3 lightDir;"
	"varying vec4 worldV;"
	"varying vec2 texCoord0;" // colorMap

	,ShaderUtils::TO_MAT3,

	"void main() {"
		"texCoord0 = vec2(texCoord.s, 1.0 - texCoord.t);" // flip texture upside down
		"worldV = M * vec4(position, 1.0);"
		"setShadowMap(worldV);"

		"vec3 N = normalize(toMat3(V) * toMat3(M) * normal);"
		"vec3 T = normalize(toMat3(V) * toMat3(M) * tangent.xyz);"
		"vec3 B = normalize(cross(N, T) * tangent.w);"
		"mat3 TBN = mat3("
			"T.x, B.x, N.x,"
			"T.y, B.y, N.y,"
			"T.z, B.z, N.z);"
		"vec3 pos = vec3(V * worldV);"
		"lightDir = normalize(TBN * (lightPosition - pos));"
		"viewDir = TBN * normalize(-pos);"
		"gl_Position = P * V * worldV;"
	"}"
};
static constexpr const char* fsColorNormalTexture[] = {
	"uniform vec4 ambientMaterial;"
	"uniform vec4 diffuseMaterial;"
	"uniform vec4 specularMaterial;"
	"uniform float shininess;"
	"uniform float alpha;"

	"uniform float ambientLight;"
	"uniform float diffuseLight;"
	"uniform float specularLight;"

	"uniform sampler2D colorMap;"
	"uniform sampler2D normalMap;"

	"varying vec3 viewDir;"
	"varying vec3 lightDir;"
	"varying vec4 worldV;"
	"varying vec2 texCoord0;"

	,ShadowMap::getFragmentShaderCode(),

	"vec4 phongModel(vec4 texColor, vec4 norm, float shadow) {"
		"vec3 r = reflect(lightDir, norm.xyz);"
		"float sDotN = max(dot(lightDir, norm.xyz), 0.0);"
		"vec4 Ka = texColor * ambientLight;"
		"vec4 Kd = texColor * diffuseLight * sDotN * shadow;"
		"vec4 Ks = sDotN > 0.0? specularMaterial * specularLight * pow(max(dot(r, viewDir), 0.0), shininess) : vec4(0.0);"
		"return Ka + Kd + Ks;"
	"}"
	"void main() {"
		"vec4 texColor = texture2D(colorMap, texCoord0);"
		"vec4 normal = texture2D(normalMap, texCoord0);"
		"float shadow = getShadow9(worldV.xyz);"
		"gl_FragColor = phongModel(texColor, normal, shadow);"
		"gl_FragColor.a = alpha;"
	"}"
};

std::unique_ptr<IShader> ModelOBJRenderer::gShaderShadow;
std::unique_ptr<IShader> ModelOBJRenderer::gShaderMaterial;
std::unique_ptr<IShader> ModelOBJRenderer::gShaderTexture;
std::unique_ptr<IShader> ModelOBJRenderer::gShaderNormalTexture;


void ModelOBJRenderer::initShaders() {
	if (!gShaderShadow.get()) {
		gShaderShadow = std::make_unique<Shader>(vsShadow, fsShadow);
		gShaderShadow->bindAttribute(0, "position");
		gShaderShadow->link();

		gShaderMaterial = std::make_unique<Shader>(vsMaterial, fsMaterial);
		gShaderMaterial->bindAttribute(0, "position");
		gShaderMaterial->bindAttribute(1, "normal");
		gShaderMaterial->link();

		gShaderTexture = std::make_unique<Shader>(vsColorTexture, fsColorTexture);
		gShaderTexture->bindAttribute(0, "position");
		gShaderTexture->bindAttribute(1, "normal");
		gShaderTexture->bindAttribute(2, "texCoord");
		gShaderTexture->link();

		gShaderNormalTexture = std::make_unique<Shader>(vsColorNormalTexture, fsColorNormalTexture);
		gShaderNormalTexture->bindAttribute(0, "position");
		gShaderNormalTexture->bindAttribute(1, "normal");
		gShaderNormalTexture->bindAttribute(2, "texCoord");
		gShaderNormalTexture->bindAttribute(3, "tangent");
		gShaderNormalTexture->link();
	}
}

//-----------------------------------------------------------------------------

ModelOBJRenderer::ModelOBJRenderer(const std::string& filename) {
	initShaders();

	mModel = std::make_unique<ModelOBJ>();
	if (!mModel->import(filename.c_str())) {
		Log::error("Failed to load model: %s", filename.c_str());
		exit(-1);
	}

	loadTextures();

	Log::debug("Model loaded: %s", filename.c_str());
	Log::debug("Textures: %d | Meshes: %d", mModelTextures.size(), mModel->getNumberOfMeshes());
}


void ModelOBJRenderer::bind() {
	int nMeshes = mModel->getNumberOfMeshes();

	auto nodeDeleter = [](RenderNode* node){
		glDeleteBuffers(1, &node->vbo);
		glDeleteBuffers(1, &node->ibo);
		Log::debug("Deleting RenderNode");
	};

	for (int i = 0; i < nMeshes; ++i) {
		const ModelOBJ::Mesh* pMesh = &mModel->getMesh(i);

		std::shared_ptr<RenderNode> node(new RenderNode, nodeDeleter);
		node->mesh = pMesh;
		node->indiceCount = 3 * pMesh->triangleCount;

		glGenBuffers(1, &node->vbo);
		glGenBuffers(1, &node->ibo);

		glBindBuffer(GL_ARRAY_BUFFER, node->vbo);
		glBufferData(GL_ARRAY_BUFFER, mModel->getNumberOfVertices() * mModel->getVertexSize(), mModel->getVertexBuffer()->position, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->ibo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, node->indiceCount * mModel->getIndexSize(), mModel->getIndexBuffer() + pMesh->startIndex, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		auto& v = pMesh->pMaterial->alpha == 1.0? mOpaqueNodes : mTranslucentNodes;
		v.push_back(node);
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	Log::debug("Model Bound | Meshes: %d | Opaque: %d | Translucent: %d", nMeshes, mOpaqueNodes.size(), mTranslucentNodes.size());
}


void ModelOBJRenderer::loadTextures() {
	const ModelOBJ::Material *pMaterial = 0;
	std::string::size_type offset = 0;

	for (int i = 0; i < mModel->getNumberOfMaterials(); ++i) {
		pMaterial = &mModel->getMaterial(i);

		// Look for and load any diffuse color map textures.
		if (pMaterial->colorMapFilename.empty())
			continue;

		std::string filename = mModel->getPath() + pMaterial->colorMapFilename;
		bool exists = IoUtils::fileExists(filename);
		if (!exists) {
			offset = pMaterial->colorMapFilename.find_last_of('\\');

			if (offset != std::string::npos)
				filename = pMaterial->colorMapFilename.substr(++offset);
			else
				filename = pMaterial->colorMapFilename;

			filename = mModel->getPath() + filename;
			exists = IoUtils::fileExists(filename);
		}

		if (exists) {
			std::shared_ptr<ITexture> texture = std::make_shared<Texture>(1, filename);
			texture->mipmap()->repeat()->unbind();
			mModelTextures[pMaterial->colorMapFilename] = texture;
		} else
			Log::error("Unable to load material %s", pMaterial->colorMapFilename.c_str());

		// Look for and load any normal map textures.

		if (pMaterial->bumpMapFilename.empty())
			continue;

		// Try load the texture using the path in the .MTL file.
		filename = mModel->getPath() + pMaterial->bumpMapFilename;
		exists = IoUtils::fileExists(filename);

		if (!exists) {
			offset = pMaterial->bumpMapFilename.find_last_of('\\');

			if (offset != std::string::npos)
				filename = pMaterial->bumpMapFilename.substr(++offset);
			else
				filename = pMaterial->bumpMapFilename;

			filename = mModel->getPath() + filename;
			exists = IoUtils::fileExists(filename);
		}

		if (exists) {
			std::shared_ptr<ITexture> texture = std::make_shared<Texture>(2, filename);
			texture->mipmap()->repeat()->unbind();
			mModelTextures[pMaterial->bumpMapFilename] = texture;
		} else
			Log::error("Unable to load material %s", pMaterial->bumpMapFilename.c_str());
	}
}


void ModelOBJRenderer::render(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const std::vector<Matrix4x4>& matrices, bool opaque) {
	IShader* shader;
	if (shadowMap->isRendering()) {
		shader = gShaderShadow.get();
		shader->run();
		shadowMap->setMatrices(gShaderShadow.get());
	}

	auto& nodes = opaque? mOpaqueNodes : mTranslucentNodes;

	for (auto& node : nodes) {
		if (!shadowMap->isRendering()) {
			const ModelOBJ::Material *pMaterial = node->mesh->pMaterial;

			if (!pMaterial->colorMapFilename.empty()) {
				if (!pMaterial->bumpMapFilename.empty()) {
					shader = gShaderNormalTexture.get();
					shader->run();
					shader->set("colorMap", mModelTextures[pMaterial->colorMapFilename].get());
					shader->set("normalMap", mModelTextures[pMaterial->bumpMapFilename].get());
				} else {
					shader = gShaderTexture.get();
					shader->run();
					shader->set("colorMap", mModelTextures[pMaterial->colorMapFilename].get());
				}
			} else {
				shader = gShaderMaterial.get();
				shader->run();
			}

			camera->setMatrices(shader);
			shadowMap->setVars(shader);

			shader->set4f("ambientMaterial", (float*) pMaterial->ambient);
			shader->set4f("diffuseMaterial", (float*) pMaterial->diffuse);
			shader->set4f("specularMaterial", (float*) pMaterial->specular);
			shader->set("shininess", pMaterial->shininess);
			shader->set("alpha", pMaterial->alpha);
			shader->set("ambientLight", sky->getAmbientLight());
			shader->set("diffuseLight", sky->getDiffuseLight());
			shader->set("specularLight", sky->getSpecularLight());
			shader->set("lightPosition", sky->getSunPosition());
		}

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, node->vbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, mModel->getVertexSize(), 0);

		if (mModel->hasNormals()) {
			glEnableVertexAttribArray(1);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->ibo);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, mModel->getVertexSize(), (GLvoid*) offsetof(ModelOBJ::Vertex, normal));
		}

		if (mModel->hasTextureCoords()) {
			glEnableVertexAttribArray(2);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->ibo);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, mModel->getVertexSize(), (GLvoid*) offsetof(ModelOBJ::Vertex, texCoord));
		}

		if (mModel->hasTangents()) {
			glEnableVertexAttribArray(3);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, node->ibo);
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, mModel->getVertexSize(), (GLvoid*) offsetof(ModelOBJ::Vertex, tangent));
		}

		for (auto& m4x4 : matrices) {
			shader->set("M", m4x4);
			glDrawElements(GL_TRIANGLES, node->indiceCount, GL_UNSIGNED_INT, 0);
		}
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	IShader::stop();
}


void ModelOBJRenderer::render(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const Matrix4x4& m4x4, bool opaque) {
	static std::vector<Matrix4x4> v(1);
	v.clear();
	v.push_back(m4x4);
	render(camera, sky, shadowMap, v, opaque);
}







