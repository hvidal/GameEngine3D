#include "GLShapeRenderer.h"
#include "ShaderUtils.h"

static constexpr const char* vsShape[] = {
	"attribute vec4 position;"
	"attribute vec4 normal;"

	"uniform mat4 V;"
	"uniform mat4 P;"

	"uniform mat4 m16;"
	"uniform vec3 sunPosition;"
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"

	,ShaderUtils::TO_MAT3,

	"void main() {"
		"vec4 worldV = m16 * position;"
		"eyeN = toMat3(V) * toMat3(m16) * normal.xyz;"
		"eyeL = V * vec4(sunPosition, 1.0);"
		"eyeV = V * worldV;"
		"gl_Position = P * V * worldV;"
	"}"
};

static constexpr const char* fsShape[] = {
	"varying vec4 eyeV;"
	"varying vec3 eyeN;"
	"varying vec4 eyeL;"

	"const float Ka = 0.15;"
	"const float Kd = 0.35;"
	"const float Ks = 0.15;"
	"const float Shininess = 0.5;"

	"vec4 color(vec3 eyeV, vec3 eyeN, vec3 eyeL) {"
		"vec3 n = normalize(eyeN);"
		"vec3 s = normalize(eyeL - eyeV);"
		"vec3 v = normalize(-eyeV);"
		"vec3 halfWay = normalize(v + s);"
		"float dotSN = abs(dot(s, eyeN));"
		"float c = Ka + Kd * max(dotSN, 0.0) + Ks * pow(max(dot(halfWay, n), 0.0), Shininess);"
		"return vec4(c, c, c, 1.0);"
	"}"
	"void main() {"
		"gl_FragColor = color(eyeV.xyz, eyeN, eyeL.xyz);"
	"}"
};

//-----------------------------------------------------------------------------


GLShapeRenderer::GLShapeRenderer() {
	mShader = std::make_unique<Shader>(vsShape, fsShape);
	mShader->bindAttribute(0, "position");
	mShader->bindAttribute(1, "normal");
	mShader->link();
}


GLShapeRenderer::~GLShapeRenderer() {
	Log::debug("Deleting GLShapeRenderer");
}


GLShapeRenderer::ShapeCache* GLShapeRenderer::cache(btConvexShape* shape) {
	ShapeCache* info = (ShapeCache*) shape->getUserPointer();
	if (!info) {
		auto pUniqueInfo = std::make_unique<ShapeCache>(shape);
		info = pUniqueInfo.get();
		shape->setUserPointer(info);
		mShapeCaches.push_front(std::move(pUniqueInfo));
	}

	if (info->mEdges.size() == 0) {
		info->mShapeHull.buildHull(shape->getMargin());

		const int ni = info->mShapeHull.numIndices();
		const int nv = info->mShapeHull.numVertices();
		const unsigned int* pi = info->mShapeHull.getIndexPointer();
		const btVector3* pv = info->mShapeHull.getVertexPointer();
		btAlignedObjectArray<ShapeCache::Edge*>	edges;
		info->mEdges.reserve(ni);
		edges.resize(nv * nv,0);
		for(int i = 0; i < ni; i += 3) {
			const unsigned int* ti = pi + i;
			const btVector3& nrm = btCross(pv[ti[1]] - pv[ti[0]], pv[ti[2]] - pv[ti[0]]).normalized();
			for(int j = 2, k = 0; k < 3; j = k++) {
				const unsigned int a = ti[j];
				const unsigned int b = ti[k];
				ShapeCache::Edge*& e = edges[btMin(a, b) * nv + btMax(a, b)];
				if (!e) {
					info->mEdges.push_back(ShapeCache::Edge());
					e = &info->mEdges[info->mEdges.size()-1];
					e->n[0] = nrm;
					e->n[1] = -nrm;
					e->v[0] = a;
					e->v[1] = b;
				} else
					e->n[1] = nrm;
			}
		}
	}
	return (info);
}

//-----------------------------------------------------------------------------


GLShapeRenderer::ShapeCache::ShapeCache(btConvexShape *s):
	mShapeHull(s),
	ready(false)
{}


GLShapeRenderer::ShapeCache::~ShapeCache() {
	Log::debug("Deleting ShaperCache");
	glDeleteBuffers(1, &mVbo);
	glDeleteBuffers(1, &mNbo);
	glDeleteBuffers(1, &mIbo);
}


void GLShapeRenderer::ShapeCache::bind() {
	int nTriangles = mShapeHull.numTriangles();

	if (nTriangles > 0) {
		const btVector3* vtx = mShapeHull.getVertexPointer();
		const unsigned int* idx = mShapeHull.getIndexPointer();

		unsigned int index = 0;
		for (unsigned int i = 0; i < nTriangles; i++) {
			unsigned int i1 = index++;
			unsigned int i2 = index++;
			unsigned int i3 = index++;
			mIndices.push_back(i1);
			mIndices.push_back(i2);
			mIndices.push_back(i3);

			unsigned int index1 = idx[i1];
			unsigned int index2 = idx[i2];
			unsigned int index3 = idx[i3];

			btVector3 v1 = vtx[index1];
			btVector3 v2 = vtx[index2];
			btVector3 v3 = vtx[index3];
			btVector3 normal = (v3-v1).cross(v2-v1).normalized();

			mVertices.push_back(v1);
			mVertices.push_back(v2);
			mVertices.push_back(v3);
			mNormals.push_back(normal);
			mNormals.push_back(normal);
			mNormals.push_back(normal);
		}
	}

	glGenBuffers(1, &mVbo);
	glGenBuffers(1, &mNbo);
	glGenBuffers(1, &mIbo);

	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(btVector3), &mVertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, mNbo);
	glBufferData(GL_ARRAY_BUFFER, mNormals.size() * sizeof(btVector3), &mNormals[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), &mIndices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	ready = true;
}


void GLShapeRenderer::ShapeCache::render() {
	if (!ready)
		bind();

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, mNbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(btVector3), 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glDrawElements(GL_TRIANGLES, mIndices.size(), GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void GLShapeRenderer::render(const ICamera *camera, const ISky *sky, const btCollisionShape* shape, const Matrix4x4& m4x4, bool opaque) {
	if (shape->getShapeType() == COMPOUND_SHAPE_PROXYTYPE) {
		const btCompoundShape* compoundShape = static_cast<const btCompoundShape*>(shape);
		for (int i = compoundShape->getNumChildShapes() - 1; i >= 0; i--) {
			float m0[16];
			compoundShape->getChildTransform(i).getOpenGLMatrix(m0);

			// we have to multiply the matrices now
			Matrix4x4 m_child(m0);
			Matrix4x4 copy(m4x4);
			copy.multiplyRight(m_child);

			const btCollisionShape* colShape = compoundShape->getChildShape(i);
			render(camera, sky, colShape, copy, opaque);
		}
		return;
	}

	if (shape->isConvex()) {
		ShapeCache* info = cache((btConvexShape*) shape);
		btShapeHull* hull = &info->mShapeHull;
		int nTriangles = hull->numTriangles();

		if (nTriangles > 0) {
			int index = 0;
			const unsigned int* idx = hull->getIndexPointer();
			const btVector3* vtx = hull->getVertexPointer();

			mShader->run();
			camera->setMatrices(mShader.get());
			mShader->set("m16", m4x4);
			mShader->set("sunPosition", sky->getSunPosition());
			info->render();

			IShader::stop();
		}
	}
}






