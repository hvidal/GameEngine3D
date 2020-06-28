#ifndef TREE_H
#define TREE_H

#include <unordered_map>
#include "../planet/Planet.h"
#include "../planet/IPlanetExternalObject.h"
#include "../../util/Pin.h"
#include "../../util/ModelOBJ.h"


class Tree: public IPlanetExternalObject {

	std::weak_ptr<Planet> mPlanet;
	std::string mTreeName;

	std::unique_ptr<IShader> mShader;
	std::unique_ptr<IShader> mShadowShader;

	std::unique_ptr<Pin> mPin;

	struct TreeData {
		btVector3 position;
		btVector3 info; // (size, rotation, texture)

		bool operator==(const TreeData& d) const
		{ return position == d.position;  }

		void write(ISerializer* serializer) const {
			serializer->write(position);
			serializer->write(info);
		}

		static TreeData read(ISerializer* serializer) {
			TreeData o;
			serializer->read(o.position);
			serializer->read(o.info);
			return o;
		}
	};

	struct TreePageData {
		std::vector<TreeData> points;
		GLuint pbo {0}; // position buffer object

		~TreePageData() {
			glDeleteBuffers(1, &pbo);
		}

		void bind() {
			if (pbo == 0) {
				glGenBuffers(1, &pbo);
			}
			glBindBuffer(GL_ARRAY_BUFFER, pbo);
			glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(TreeData), &points[0].position, GL_STREAM_DRAW);
		}

		void write(ISerializer* serializer) const {
			serializer->write(points.size());
			for (auto& data : points)
				data.write(serializer);
		}

		static std::unique_ptr<TreePageData> read(ISerializer* serializer) {
			std::vector<TreeData>::size_type vectorSize;
			serializer->read(vectorSize);
			std::unique_ptr<TreePageData> o = std::make_unique<TreePageData>();
			for (std::vector<TreeData>::size_type i = 0; i < vectorSize; ++i) {
				o->points.push_back(TreeData::read(serializer));
			}
			return o;
		}
	};

	struct MeshData {
		GLuint ibo;
		bool hasWind;
		unsigned int indexCount;
		std::unique_ptr<ITexture> texture;

		~MeshData() { glDeleteBuffers(1, &ibo); }
	};

	struct ModelData {
		std::string filename;
		unsigned int windMesh;
		std::unique_ptr<ModelOBJ> modelOBJ;
		std::vector<std::unique_ptr<MeshData>> meshes;
		GLuint vbo;

		ModelData(const std::string&, unsigned int windMesh);
		~ModelData();

		void render(const std::vector<TreePageData*>& pageData, IShader* shader);

		void write(ISerializer* serializer) const;
		static std::unique_ptr<ModelData> read(ISerializer* serializer);
	};

	struct ModelGroup {
		std::map<float, std::unique_ptr<ModelData>> modelLOD;
		std::map<unsigned int, std::unique_ptr<TreePageData>> pagePoints;

		void render(const std::unordered_map<unsigned int,float>& pageIds, IShader* shader);

		void write(ISerializer* serializer) const;
		static std::unique_ptr<ModelGroup> read(ISerializer* serializer);
	};

	std::unordered_map<std::string, std::unique_ptr<ModelGroup>> mModelGroups;

	void addTrees(const btVector3& point, const std::string& treeName);
	void removeTrees(const btVector3& point);

	void render(const std::unordered_map<unsigned int,float>& pageIds, const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap);
public:
	static const std::string& SERIALIZE_ID;

	Tree(std::weak_ptr<Planet> planet);
	virtual ~Tree();

	void addModel(const std::string& name, const std::string& filename, float maxDistance, unsigned int windMesh);

	virtual ISceneObject::CommandMap getCommands();
	virtual void renderOpaque(const std::unordered_map<unsigned int,float>& pageIds, const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const GameState &gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

#endif
