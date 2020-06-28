#pragma once

#include <forward_list>
#include "LinearMath/btVector3.h"
#include "BulletCollision/CollisionShapes/btShapeHull.h"

#include "math/Matrix4x4.h"
#include "Shader.h"


class GLShapeRenderer
{
private:
	struct ShapeCache {
		bool ready;
		GLuint mVbo;
		GLuint mNbo;
		GLuint mIbo;

		std::vector<unsigned int> mIndices;
		std::vector<btVector3> mVertices;
		std::vector<btVector3> mNormals;

		struct Edge {
			btVector3 n[2];
			int v[2];
		};

		btShapeHull mShapeHull;
		btAlignedObjectArray<Edge> mEdges;

		ShapeCache(btConvexShape*);
		~ShapeCache();

		void bind();
		void render();
	};

	std::forward_list<std::unique_ptr<ShapeCache>> mShapeCaches;
	std::unique_ptr<IShader> mShader;

	ShapeCache* cache(btConvexShape* shape);

public:
	GLShapeRenderer();
	virtual ~GLShapeRenderer();

	void render(const ICamera *camera, const ISky *sky, const btCollisionShape* shape, const Matrix4x4& m4x4, bool opaque);
};

//-----------------------------------------------------------------------------

