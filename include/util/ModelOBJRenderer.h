#ifndef INCLUDE_UTIL_MODEL_OBJ_RENDERER_H_
#define INCLUDE_UTIL_MODEL_OBJ_RENDERER_H_

#include <string>
#include <unordered_map>
#include <vector>

#include "LinearMath/btVector3.h"

#include "ModelOBJ.h"
#include "Texture.h"
#include "Shader.h"
#include "IoUtils.h"
#include "ShadowMap.h"
#include "ShaderUtils.h"
#include "math/Matrix4x4.h"


class ModelOBJRenderer {

	static std::unique_ptr<IShader> gShaderShadow;
	static std::unique_ptr<IShader> gShaderMaterial;
	static std::unique_ptr<IShader> gShaderTexture;
	static std::unique_ptr<IShader> gShaderNormalTexture;
	static void initShaders();

	struct RenderNode {
		GLuint vbo;
		GLuint ibo;
		GLuint indiceCount;
		const ModelOBJ::Mesh* mesh;
	};

	std::unique_ptr<ModelOBJ> mModel;
	std::map<std::string, std::shared_ptr<ITexture>> mModelTextures;
	std::vector<std::shared_ptr<RenderNode>> mOpaqueNodes;
	std::vector<std::shared_ptr<RenderNode>> mTranslucentNodes;

	void loadTextures();
public:
	explicit ModelOBJRenderer(const std::string& filename);

	void bind();

	void render(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const Matrix4x4& m4x4, bool opaque);
	void render(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const std::vector<Matrix4x4>& matrices, bool opaque);
};

//-----------------------------------------------------------------------------

static std::unordered_map<std::string, std::shared_ptr<ModelOBJRenderer>> sCache;

// cache
class ModelOBJRendererCache {
public:
	static std::shared_ptr<ModelOBJRenderer> get(const std::string& filename) {
		if (sCache.count(filename) == 0) {
			std::shared_ptr<ModelOBJRenderer> o(std::make_shared<ModelOBJRenderer>(IoUtils::resource(filename)));
			o->bind();
			sCache[filename] = o;
			Log::debug("[%d] Renderer created for %s", sCache.size(), filename.c_str());
			return o;
		}
		return sCache.at(filename);
	}
};


#endif
