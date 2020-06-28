#ifndef MODEL_OBJ_H
#define MODEL_OBJ_H

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <OpenGL/gl3.h>
#include "LinearMath/btVector3.h"

//-----------------------------------------------------------------------------
// Alias|Wavefront OBJ file loader.
//
// This OBJ file loader contains the following restrictions:
// 1. Group information is ignored. Faces are grouped based on the material
//    that each face uses.
// 2. Object information is ignored. This loader will merge everything into a
//    single object.
// 3. The MTL file must be located in the same directory as the OBJ file. If
//    it isn't then the MTL file will fail to load and a default material is
//    used instead.
// 4. This loader triangulates all polygonal faces during importing.
//-----------------------------------------------------------------------------

class ModelOBJ
{
public:
	struct Material
	{
		float ambient[4];
		float diffuse[4];
		float specular[4];
		float shininess;        // [0 = min shininess, 1 = max shininess]
		float alpha;            // [0 = fully transparent, 1 = fully opaque]

		std::string name;
		std::string colorMapFilename;
		std::string bumpMapFilename;
	};

	struct Vertex
	{
		float position[3];
		float texCoord[2];
		float normal[3];
		float tangent[4];
		float bitangent[3];
	};

	struct Mesh
	{
		unsigned int startIndex;
		unsigned int triangleCount;
		const Material *pMaterial;
	};

	ModelOBJ();
	~ModelOBJ();

	void destroy();
	bool import(const char *pszFilename, bool rebuildNormals = false);
	void reverseWinding();

	// Getter methods.

	void getCenter(float &x, float &y, float &z) const noexcept;
	float getWidth() const noexcept;
	float getHeight() const noexcept;
	float getLength() const noexcept;
	float getRadius() const noexcept;

	const int *getIndexBuffer() const;
	constexpr int getIndexSize() const;

	const Material &getMaterial(int i) const;
	const Mesh &getMesh(int i) const;

	unsigned int getNumberOfIndices() const noexcept;
	unsigned int getNumberOfMaterials() const noexcept;
	unsigned int getNumberOfMeshes() const noexcept;
	unsigned int getNumberOfTriangles() const noexcept;
	unsigned int getNumberOfVertices() const noexcept;
	unsigned int getNumberOfEdges() const noexcept;

	const std::string &getPath() const;

	const Vertex &getVertex(int i) const;
	const Vertex *getVertexBuffer() const;
	constexpr int getVertexSize() const;

	bool hasNormals() const noexcept;
	bool hasPositions() const noexcept;
	bool hasTangents() const noexcept;
	bool hasTextureCoords() const noexcept;

	void scale(float dx, float dy, float dz);
	void translate(float dx, float dy, float dz);
	void rotateY(float angle);

private:
	void addTrianglePos(int index, int material, int v0, int v1, int v2);
	void addTrianglePosNormal(int index, int material, int v0, int v1, int v2, int vn0, int vn1, int vn2);
	void addTrianglePosTexCoord(int index, int material, int v0, int v1, int v2, int vt0, int vt1, int vt2);
	void addTrianglePosTexCoordNormal(int index, int material, int v0, int v1, int v2, int vt0, int vt1, int vt2, int vn0, int vn1, int vn2);
	int addVertex(int hash, const Vertex *pVertex);
	void bounds(float center[3], float &width, float &height, float &length, float &radius) const;
	void buildMeshes();
	void generateNormals();
	void generateTangents();
	void importGeometryFirstPass(FILE *pFile);
	void importGeometrySecondPass(FILE *pFile);
	bool importMaterials(const char *pszFilename);

	bool m_hasPositions;
	bool m_hasTextureCoords;
	bool m_hasNormals;
	bool m_hasTangents;

	unsigned int m_numberOfVertexCoords;
	unsigned int m_numberOfTextureCoords;
	unsigned int m_numberOfNormals;
	unsigned int m_numberOfTriangles;
	unsigned int m_numberOfMaterials;
	unsigned int m_numberOfMeshes;
	unsigned int m_numberOfEdges;

	float m_center[3];
	float m_width;
	float m_height;
	float m_length;
	float m_radius;

	std::string m_directoryPath;

	std::vector<Mesh> m_meshes;
	std::vector<Material> m_materials;
	std::vector<Vertex> m_vertexBuffer;
	std::vector<int> m_indexBuffer;
	std::vector<int> m_attributeBuffer;
	std::vector<float> m_vertexCoords;
	std::vector<float> m_textureCoords;
	std::vector<float> m_normals;

	std::map<std::string, int> m_materialCache;
	std::map<int, std::vector<int> > m_vertexCache;
};

//-----------------------------------------------------------------------------

inline void ModelOBJ::getCenter(float &x, float &y, float &z) const noexcept
{ x = m_center[0]; y = m_center[1]; z = m_center[2]; }

inline float ModelOBJ::getWidth() const noexcept
{ return m_width; }

inline float ModelOBJ::getHeight() const noexcept
{ return m_height; }

inline float ModelOBJ::getLength() const noexcept
{ return m_length; }

inline float ModelOBJ::getRadius() const noexcept
{ return m_radius; }

inline const int *ModelOBJ::getIndexBuffer() const
{ return &m_indexBuffer[0]; }

inline constexpr int ModelOBJ::getIndexSize() const
{ return static_cast<int>(sizeof(int)); }

inline const ModelOBJ::Material &ModelOBJ::getMaterial(int i) const
{ return m_materials[i]; }

inline const ModelOBJ::Mesh &ModelOBJ::getMesh(int i) const
{ return m_meshes[i]; }

inline unsigned int ModelOBJ::getNumberOfIndices() const noexcept
{ return m_numberOfTriangles * 3; }

inline unsigned int ModelOBJ::getNumberOfMaterials() const noexcept
{ return m_numberOfMaterials; }

inline unsigned int ModelOBJ::getNumberOfMeshes() const noexcept
{ return m_numberOfMeshes; }

inline unsigned int ModelOBJ::getNumberOfTriangles() const noexcept
{ return m_numberOfTriangles; }

inline unsigned int ModelOBJ::getNumberOfVertices() const noexcept
{ return static_cast<unsigned int>(m_vertexBuffer.size()); }

inline const std::string &ModelOBJ::getPath() const
{ return m_directoryPath; }

inline const ModelOBJ::Vertex &ModelOBJ::getVertex(int i) const
{ return m_vertexBuffer[i]; }

inline const ModelOBJ::Vertex *ModelOBJ::getVertexBuffer() const
{ return &m_vertexBuffer[0]; }

inline constexpr int ModelOBJ::getVertexSize() const
{ return static_cast<int>(sizeof(Vertex)); }

inline bool ModelOBJ::hasNormals() const noexcept
{ return m_hasNormals; }

inline bool ModelOBJ::hasPositions() const noexcept
{ return m_hasPositions; }

inline bool ModelOBJ::hasTangents() const noexcept
{ return m_hasTangents; }

inline bool ModelOBJ::hasTextureCoords() const noexcept
{ return m_hasTextureCoords; }

#endif
