#include <btBulletDynamicsCommon.h>
#include <array>
#include <chrono>
#include "PlanetPage.h"
#include "../../util/math/Plane.h"


const static btVector3 PLANET_CENTER(0.f, 0.f, 0.f);


/* private constructor */
PlanetPage::PlanetPage(unsigned int pageId):
	mPageId(pageId)
{}


PlanetPage::PlanetPage(unsigned int pageId, const std::string& plane, float radius, float waterLevel, float d1, float d2, float size, float face, unsigned int pageDivisions):
	mPageId(pageId),
	mPlanetRadius(radius),
	mWaterLevel(waterLevel),
	mHasWater(false),
	mIsActive(false)
{
	if (pageDivisions % 2 == 1)
		throw std::runtime_error("pageDivisions must be even, but it is " + std::to_string(pageDivisions));

	buildDetailedMesh(plane, radius, d1, d2, size, face, pageDivisions);
	buildSimplifiedMesh(plane, pageDivisions);
	setFieldOfView();
}


PlanetPage::~PlanetPage() {
	glDeleteBuffers(1, &mVbo);
	glDeleteBuffers(1, &mIboDetailed);
	glDeleteBuffers(1, &mIboSimplified);
}


btVector3 PlanetPage::getInnerPoint(const ICamera *camera, float planetRadius) {
	// Finds the point under the camera that is aligned with the horizon
	// The height of the camera is the hypotenuse
	// The radius is the opposite side and distanceToHorizon is the adjacent side
	float cameraHeight = camera->getPosition().length();
	double distanceToHorizon = sqrt(cameraHeight * cameraHeight - planetRadius * planetRadius);
	double angle = asin(planetRadius / cameraHeight);
	double distanceCameraToInnerPoint = cos(angle) * distanceToHorizon;
	// Reduces 5% of the length in order to avoid glitches at the horizon
	double innerLength = 0.95 * (cameraHeight - distanceCameraToInnerPoint);
	return innerLength * camera->getPosition().normalized();
}


btVector3 PlanetPage::getCorner(unsigned int index) const noexcept {
	return mVertices[mCornerIndex[index]].position;
}


void PlanetPage::setVertex(const std::string& plane, float radius, float a, float b, float face, PlanetPageVertex& vertex) {
	float x, y, z;
	float shift = plane[0] == '+'? face : -face;
	if (plane[1] == 'x' && plane[2] == 'y') { // XY plane
		x = a;
		y = b;
		z = shift;
	} else if (plane[1] == 'z' && plane[2] == 'x') { // ZY plane
		x = b;
		y = shift;
		z = a;
	} else if (plane[1] == 'y' && plane[2] == 'z') { // YZ plane
		x = shift;
		y = a;
		z = b;
	} else
		throw std::runtime_error("Invalid plane specification: " + plane);

	// Move point to the surface of the sphere
	vertex.normal = btVector3(x, y, z).normalized();
	vertex.position = vertex.normal * radius;
	vertex.material[0] = 1.f;
	vertex.material[1] = 0.f;
	vertex.material[2] = 0.f;
	vertex.material[3] = 0.f;
	vertex.uv[0] = a / face;
	vertex.uv[1] = b / face;
}


void PlanetPage::buildDetailedMesh(const std::string& plane, float radius, float d1, float d2, float size, float face, unsigned int pageDivisions) {
	unsigned int dotsPerSide = pageDivisions + 1;
	mVerticeCount = dotsPerSide * dotsPerSide;
	mTriangleCount = 2 * pageDivisions * pageDivisions;
	unsigned int indiceCount = mTriangleCount * 3;

	mVertices.reserve(mVerticeCount);
	mIndicesDetailed.reserve(indiceCount);
	mBorderIndices.reserve(4 * pageDivisions);

	const unsigned int centerIndex = pageDivisions / 2;
	const float pageDivisionsF = static_cast<float>(pageDivisions);

	for (int a = 0; a < dotsPerSide; a++) {
		float dA = size * a / pageDivisionsF;

		for (int b = 0; b < dotsPerSide; b++) {
			float dB = size * b / pageDivisionsF;

			PlanetPageVertex vertex{};
			setVertex(plane, radius, d1 + dA, d2 + dB, face, vertex);
			mVertices.push_back(vertex);
			const unsigned long currentIndex = mVertices.size() - 1;

			if (a == 0 || b == 0 || a == pageDivisions || b == pageDivisions) {
				unsigned int index = static_cast<unsigned int>(mVertices.size() - 1);
				mBorderIndices.push_back(index);

				if (a == 0 && b == 0) {
					mCornerIndex[0] = currentIndex;
				} else if (a == 0 && b == pageDivisions) {
					mCornerIndex[1] = currentIndex;
				} else if (a == pageDivisions && b == pageDivisions) {
					mCornerIndex[2] = currentIndex;
				} else if (a == pageDivisions && b == 0) {
					mCornerIndex[3] = currentIndex;
				}
			} else if (a == centerIndex && b == centerIndex)
				mCenterIndex = currentIndex;
		}
	}

	mCenterDirection1 = (getCorner(0) + getCorner(1) + getCorner(2) + getCorner(3)).normalized();
	mDotToCenterLimit = getCorner(0).normalized().dot(getCorner(2).normalized());

	auto addTriangle = [this, &plane, &dotsPerSide](std::vector<unsigned int>&& v) {
		if (plane[0] == '+') {
			mIndicesDetailed.push_back(v[0] * dotsPerSide + v[1]);
			mIndicesDetailed.push_back(v[2] * dotsPerSide + v[3]);
			mIndicesDetailed.push_back(v[4] * dotsPerSide + v[5]);
		} else {
			mIndicesDetailed.push_back(v[4] * dotsPerSide + v[5]);
			mIndicesDetailed.push_back(v[2] * dotsPerSide + v[3]);
			mIndicesDetailed.push_back(v[0] * dotsPerSide + v[1]);
		}
	};

	for (unsigned int a = 0; a < pageDivisions; a++) {
		for (unsigned int b = 0; b < pageDivisions; b++) {
			addTriangle({a+1, b+1, a, b+1, a, b});
			addTriangle({a+1, b, a+1, b+1, a, b});
		}
	}

	// NORMALS
	calculateNormals(false);
}


void PlanetPage::buildSimplifiedMesh(const std::string& plane, unsigned int pageDivisions) {
	unsigned int dotsPerSide = pageDivisions + 1;
	unsigned int simpleSectionsPerSide = (pageDivisions / 2) - 1; // number of big triangles per side
	unsigned int simpleTriangleCount =
			(2 /*2 triangles per merged area*/ * simpleSectionsPerSide * simpleSectionsPerSide) +
			(3 /*border triangles*/ * simpleSectionsPerSide * 4 /*sides*/) +
			(4 /*corners*/ * 2 /*triangles*/);

	unsigned int simpleIndiceCount = simpleTriangleCount * 3;
	mIndicesSimplified.reserve(simpleIndiceCount);

	auto addTriangle = [this, &plane, &dotsPerSide](std::vector<unsigned int>&& v) {
		if (plane[0] == '+') {
			mIndicesSimplified.push_back(v[0] * dotsPerSide + v[1]);
			mIndicesSimplified.push_back(v[2] * dotsPerSide + v[3]);
			mIndicesSimplified.push_back(v[4] * dotsPerSide + v[5]);
		} else {
			mIndicesSimplified.push_back(v[4] * dotsPerSide + v[5]);
			mIndicesSimplified.push_back(v[2] * dotsPerSide + v[3]);
			mIndicesSimplified.push_back(v[0] * dotsPerSide + v[1]);
		}
	};

	// Four Corners
	for (unsigned int a = 0; a < dotsPerSide - 1; a += dotsPerSide - 2) {
		for (unsigned int b = 0; b < dotsPerSide - 1; b += dotsPerSide - 2) {
			addTriangle({a, b, a+1, b+1, a, b+1});
			addTriangle({a, b, a+1, b, a+1, b+1});
		}
	}

	// Inner simplified meshes
	for (unsigned int a = 1; a < dotsPerSide - 3; a += 2) {
		for (unsigned int b = 1; b < dotsPerSide - 3; b += 2) {
			addTriangle({a, b, a+2, b+2, a, b+2});
			addTriangle({a, b, a+2, b, a+2, b+2});
		}
	}

	// Border simplified triangles (along B axis)
	for (unsigned int b = 1; b < dotsPerSide - 3; b += 2) {
		// one side
		unsigned int a = 0;
		addTriangle({a+1, b, a, b+1, a, b});
		addTriangle({a+1, b, a+1, b+2, a, b+1});
		addTriangle({a+1, b+2, a, b+2, a, b+1});

		// other side
		a = dotsPerSide - 2;
		addTriangle({a+1, b, a+1, b+1, a, b});
		addTriangle({a+1, b+1, a, b+2, a, b});
		addTriangle({a+1, b+1, a+1, b+2, a, b+2});
	}

	// Border simplified triangles (along A axis)
	for (unsigned int a = 1; a < dotsPerSide - 3; a += 2) {
		// one side
		unsigned int b = 0;
		addTriangle({a+1, b, a, b+1, a, b});
		addTriangle({a+1, b, a+2, b+1, a, b+1});
		addTriangle({a+2, b, a+2, b+1, a+1, b});

		b = dotsPerSide - 2;
		addTriangle({a, b, a+1, b+1, a, b+1});
		addTriangle({a, b, a+2, b, a+1, b+1});
		addTriangle({a+2, b, a+2, b+1, a+1, b+1});
	}
}


void PlanetPage::calculateNormals(bool reset) {
	if (reset) {
		std::for_each(mVertices.begin(), mVertices.end(), [](PlanetPageVertex& v) {
			v.normal.setZero();
		});
	}
	for (unsigned int i = 0; i < mIndicesDetailed.size(); i += 3)
	{
		PlanetPageVertex& v0 = mVertices[mIndicesDetailed[i]];
		PlanetPageVertex& v1 = mVertices[mIndicesDetailed[i+1]];
		PlanetPageVertex& v2 = mVertices[mIndicesDetailed[i+2]];

		const btVector3& p0 = v0.position;
		const btVector3& p1 = v1.position;
		const btVector3& p2 = v2.position;
		btVector3 normal = btCross(p1 - p0, p2 - p0);
		if (normal.dot(p0 + p1 + p2) < 0.f)
			normal = -normal;

		v0.normal += normal;
		v0.normal.normalize();

		v1.normal += normal;
		v1.normal.normalize();

		v2.normal += normal;
		v2.normal.normalize();
	}
}


void PlanetPage::setFieldOfView() {
	mFOD = std::make_unique<FieldOfView>(PLANET_CENTER, getCorner(0), getCorner(1), getCorner(2), getCorner(3));
}


void PlanetPage::initPhysics(btDynamicsWorld* dynamicsWorld) {
	bind();

	constexpr int vertexStride = sizeof(PlanetPageVertex);
	constexpr int indexStride = 3 * sizeof(unsigned int);

	std::unique_ptr<btTriangleIndexVertexArray> puIndexVertexArrays = std::make_unique<btTriangleIndexVertexArray>(
			mTriangleCount,
			reinterpret_cast<int*> (&mIndicesDetailed[0]),
			indexStride,
			mVerticeCount,
			const_cast<float*>(&mVertices[0].position.x()),
			vertexStride);

	static const btVector3 localInertia(0,0,0);

	std::unique_ptr<btMotionState> puMotionState = std::make_unique<btDefaultMotionState>();

	mPhysicsBody = std::make_unique<PhysicsBody>(0.f, std::move(puIndexVertexArrays), std::move(puMotionState), localInertia);
	mPhysicsBody->getCollisionShape()->setMargin(0.5f);

	btScalar defaultContactProcessingThreshold(BT_LARGE_FLOAT);
	mPhysicsBody->getRigidBody()->setContactProcessingThreshold(defaultContactProcessingThreshold);

	dynamicsWorld->addRigidBody(mPhysicsBody->getRigidBody());
	//mPhysicsBody->getRigidBody()->setFriction(0.0f);
	//mPhysicsBody->getRigidBody()->setHitFraction(0.8f);
	//mPhysicsBody->getRigidBody()->setRestitution(0.6f);

	mIsActive = true;
}

void PlanetPage::bind() {
	glGenBuffers(1, &mVbo);
	glGenBuffers(1, &mIboDetailed);
	glGenBuffers(1, &mIboSimplified);

	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, mVerticeCount * sizeof(PlanetPageVertex), &mVertices[0].position, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIboDetailed);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesDetailed.size() * sizeof(unsigned int), &mIndicesDetailed[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIboSimplified);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesSimplified.size() * sizeof(unsigned int), &mIndicesSimplified[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


static constexpr const float SIMPLIFIED_DISTANCE = 1000.0f;

void PlanetPage::renderOpaque(float distanceToCamera, const GameState& gameState) {
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PlanetPageVertex), 0);

	bool isSimplified = distanceToCamera > SIMPLIFIED_DISTANCE || gameState.debugCode == DebugCode::SIMPLIFIED;
	GLuint ibo = isSimplified? mIboSimplified : mIboDetailed;
	const size_t indiceCount = isSimplified? mIndicesSimplified.size() : mIndicesDetailed.size();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PlanetPageVertex), (GLvoid*) offsetof(PlanetPageVertex, normal));
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(PlanetPageVertex), (GLvoid*) offsetof(PlanetPageVertex, material));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(PlanetPageVertex), (GLvoid*) offsetof(PlanetPageVertex, uv));

	glDrawElements(GL_TRIANGLES, static_cast<GLsizei> (indiceCount), GL_UNSIGNED_INT, 0);
}


void PlanetPage::renderTranslucent(float distanceToCamera, const GameState& gameState) {
	if (mHasWater) {
		glBindBuffer(GL_ARRAY_BUFFER, mVbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PlanetPageVertex), 0);

		bool isSimplified = distanceToCamera > SIMPLIFIED_DISTANCE || gameState.debugCode == DebugCode::SIMPLIFIED;
		GLuint ibo = isSimplified? mIboSimplified : mIboDetailed;
		const size_t indiceCount = isSimplified? mIndicesSimplified.size() : mIndicesDetailed.size();

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(PlanetPageVertex), (GLvoid*) offsetof(PlanetPageVertex, uv));

		glDrawElements(GL_TRIANGLES, static_cast<GLsizei> (indiceCount), GL_UNSIGNED_INT, 0);
	}
}


bool PlanetPage::isVisible(const ICamera* camera, const btVector3& innerPoint) const {
	if (mCameraVisibility.first == camera->getVersion()) {
		return mCameraVisibility.second;
	}

	const auto& cameraPosition = camera->getPosition();
	const auto& center = mVertices[mCenterIndex].position;

	bool isInside = mFOD->isVisible(cameraPosition);
	bool isInFrontAndCloseEnough = camera->isInFront(center) && isCloseTo(cameraPosition);
	if (isInside || isInFrontAndCloseEnough) {
		mCameraVisibility.first = camera->getVersion();
		mCameraVisibility.second = true;
		return true;
	}

	bool isVisible = false;
	const btVector3& vInnerToCamera = cameraPosition - innerPoint;
	if (vInnerToCamera.dot(getCorner(0) - innerPoint) >= 0.0 ||
		vInnerToCamera.dot(getCorner(1) - innerPoint) >= 0.0 ||
		vInnerToCamera.dot(getCorner(2) - innerPoint) >= 0.0 ||
		vInnerToCamera.dot(getCorner(3) - innerPoint) >= 0.0)
	{
		// Here we check if at least one of the corners is visible to the camera
		isVisible = camera->isVisible(getCorner(0)) ||
					camera->isVisible(getCorner(1)) ||
					camera->isVisible(getCorner(2)) ||
					camera->isVisible(getCorner(3));
	}
	mCameraVisibility.first = camera->getVersion();
	mCameraVisibility.second = isVisible;

	return isVisible;
}


bool PlanetPage::isCloseTo(const btVector3& point) const {
	float dotPointToCenter = point.normalized().dot(mCenterDirection1);
	return dotPointToCenter > mDotToCenterLimit;
}

float PlanetPage::arcDistanceTo(const btVector3& point) const {
	const double cosAngleToCenter = point.normalized().dot(mCenterDirection1);
	double angleRad = acos(cosAngleToCenter);
	return static_cast<float>(mPlanetRadius * angleRad);
}


void PlanetPage::enablePhysics(btDynamicsWorld *dynamicsWorld, const std::forward_list<std::shared_ptr<btRigidBody>>& rigidBodies) {
	bool isCloseEnough = false;
	for (auto& body : rigidBodies) {
		if (isCloseTo(body->getCenterOfMassPosition())) {
			isCloseEnough = true;
			break;
		}
	}

	if (isCloseEnough && !mIsActive) {
		mIsActive = true;
		dynamicsWorld->addRigidBody(mPhysicsBody->getRigidBody());
	} else if (!isCloseEnough && mIsActive) {
		mIsActive = false;
		dynamicsWorld->removeRigidBody(mPhysicsBody->getRigidBody());
	}
}


const static auto random0 = []() -> float {
	return static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
};


void PlanetPage::editVertices(const std::string& command, float value, const btVector3& point3D, float brushSize) {
	bool isModified = false;
	if (command == "terrain") {
		mHasWater = false;
		for (PlanetPageVertex& vertex : mVertices) {
			float distance = point3D.distance(vertex.position);
			if (distance <= brushSize) {
				btVector3 up = vertex.position.normalized();
				float factor = value * (1.f - distance / brushSize);
				btVector3 v = vertex.position + up * factor;
				vertex.position.setValue(v.x(), v.y(), v.z());
				isModified = true;
			}
			if (vertex.position.length() < mWaterLevel)
				mHasWater = true;
		}
		if (isModified) {
			calculateNormals(true);
		}
	} else if (command == "mat0" || command == "mat1" || command == "mat2" || command == "mat3") {
		int index = command == "mat0"? 0 : command == "mat1"? 1 : command == "mat2"? 2 : 3;
		for (PlanetPageVertex& vertex : mVertices) {
			if (random0() > 0.35f)
				continue;
			float distance = point3D.distance(vertex.position);
			if (distance <= brushSize) {
				vertex.material[index] = value;
				isModified = true;
			}
		}
	}
	if (isModified) {
		glBindBuffer(GL_ARRAY_BUFFER, mVbo);
		glBufferData(GL_ARRAY_BUFFER, mVerticeCount * sizeof(PlanetPageVertex), &mVertices[0].position, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


void PlanetPage::autoPaintVertices(const std::string& param, const btVector3& point3D, float brushSize) {
	bool isModified = false;
	for (PlanetPageVertex& vertex : mVertices) {
		float distance = point3D.distance(vertex.position);
		if (distance <= brushSize) {
			btVector3 up = vertex.position.normalized();
			float dot = up.dot(vertex.normal);
			int matA = param[0] - '0';
			int matB = param[1] - '0';
			vertex.material[0] = vertex.material[1] = vertex.material[2] = vertex.material[3] = 0.f;
			vertex.material[matA] = dot;
			vertex.material[matB] = 1 - dot;
			isModified = true;
		}
	}
	if (isModified) {
		glBindBuffer(GL_ARRAY_BUFFER, mVbo);
		glBufferData(GL_ARRAY_BUFFER, mVerticeCount * sizeof(PlanetPageVertex), &mVertices[0].position, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


float PlanetPage::getHeightAt(const btVector3& direction) const {

	bool isVisible = mFOD->isVisible(direction);

	// If inside this page, then search inside each triangle
	if (isVisible) {
		const Line line(PLANET_CENTER, direction);
		btVector3 interceptionPoint;

		const float maxDistanceBetweenVertices = 2.0f * mVertices[0].position.distance(mVertices[1].position);

		for (unsigned long i = 0; i < mIndicesDetailed.size(); i += 3) {
			const btVector3& p0 = mVertices[mIndicesDetailed[i]].position;
			if (line.distanceTo(p0) > maxDistanceBetweenVertices)
				continue;
			
			const btVector3& p1 = mVertices[mIndicesDetailed[i+1]].position;
			const btVector3& p2 = mVertices[mIndicesDetailed[i+2]].position;

			const Triangle triangle(p0, p1, p2);
			bool isInside = triangle.isInterceptedBy(line, interceptionPoint);
			if (isInside)
				return interceptionPoint.length();
		}
	}
	return -1.0f;
}


void PlanetPage::fixBorderNormals(const std::function<void(const btVector3&,btVector3&)>& getNormalAt, const btVector3& mouse3d, float brushSize, const ICamera* camera, const btVector3& innerPoint) {
	if (isVisible(camera, innerPoint)) {
		for (unsigned int i : mBorderIndices) {
			PlanetPageVertex& v = mVertices[i];
			if (v.position.distance(mouse3d) <= brushSize) {
				btVector3 normal(0.f, 0.f, 0.f);
				getNormalAt(v.position, normal);
				v.normal = normal.normalized();
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, mVbo);
		glBufferData(GL_ARRAY_BUFFER, mVerticeCount * sizeof(PlanetPageVertex), &mVertices[0].position, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
}


void PlanetPage::getNormalAt(const btVector3& point, btVector3& normal) const {
	bool isVisible = mFOD->isVisible(point);
	if (isVisible) {
		for (const PlanetPageVertex& v : mVertices) {
			if (v.position.distance(point) <= 0.1f) {
				btVector3 n = normal + v.normal;
				normal.setValue(n.x(), n.y(), n.z());
				return;
			}
		}
	}
}


unsigned int PlanetPage::getPageId(const btVector3 &point) const noexcept {
	return mFOD->isVisible(point)? mPageId : 0;
}


void PlanetPage::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), 0);

	serializer->write(mPageId);
	serializer->write(mPlanetRadius);
	serializer->write(mWaterLevel);
	serializer->write(mHasWater);
	serializer->write(mIsActive);
	serializer->write(mCenterDirection1);
	serializer->write(mDotToCenterLimit);
	serializer->write(mTriangleCount);
	serializer->write(mVerticeCount);

	for (const PlanetPageVertex& v : mVertices) {
		v.write(serializer);
	}

	serializer->write(mIndicesDetailed);
	serializer->write(mIndicesSimplified);
	serializer->write(mBorderIndices);
	serializer->write(mCenterIndex);
	serializer->write(mCornerIndex[0]);
	serializer->write(mCornerIndex[1]);
	serializer->write(mCornerIndex[2]);
	serializer->write(mCornerIndex[3]);
}


std::unique_ptr<PlanetPage> PlanetPage::create(ISerializer *serializer, std::weak_ptr<btDynamicsWorld> dynamicsWorld) {
	std::string className;
	unsigned long objectId;
	serializer->readBegin(className, objectId);
	if (className != serializeID() || objectId != 0)
		throw std::runtime_error("Not a planet page: " + className);

	Log::debug("CREATING %s", className.c_str());

	unsigned int pageId;
	serializer->read(pageId);

	// calling private constructor, cannot call std::make_unique
	std::unique_ptr<PlanetPage> o = std::unique_ptr<PlanetPage>(new PlanetPage(pageId));
	serializer->read(o->mPlanetRadius);
	serializer->read(o->mWaterLevel);
	serializer->read(o->mHasWater);
	serializer->read(o->mIsActive);
	serializer->read(o->mCenterDirection1);
	serializer->read(o->mDotToCenterLimit);
	serializer->read(o->mTriangleCount);
	serializer->read(o->mVerticeCount);

	o->mVertices.reserve(o->mVerticeCount);
	for (int i = 0; i < o->mVerticeCount; i++) {
		o->mVertices.push_back(PlanetPageVertex::read(serializer));
	}

	serializer->read(o->mIndicesDetailed);
	serializer->read(o->mIndicesSimplified);
	serializer->read(o->mBorderIndices);
	serializer->read(o->mCenterIndex);
	serializer->read(o->mCornerIndex[0]);
	serializer->read(o->mCornerIndex[1]);
	serializer->read(o->mCornerIndex[2]);
	serializer->read(o->mCornerIndex[3]);

	o->calculateNormals(false);
	o->setFieldOfView();

	return o;
}


std::string PlanetPage::serializeID() {
	return "PlanetPage";
}




