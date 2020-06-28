#include "PlanetFace.h"

const static btVector3 PLANET_CENTER(0.f, 0.f, 0.f);


/* private constructor */
PlanetFace::PlanetFace() {}


PlanetFace::PlanetFace(unsigned int faceId, const std::string& plane, float radius, float waterLevel, unsigned int faceDivisions, unsigned int pageDivisions)
{
	if (faceDivisions % 2 == 0)
		throw std::runtime_error("faceDivisions must be odd, but it is " + std::to_string(faceDivisions));

	// This is the distance from the origin that the camera should consider when checking for visible faces
	// If the cube is aligned with each axis, this is the diagonal projected on the axis
	// Pitagoras: Radius (R) is the hypotenuse (origin to cube vertex)
	// The other sides are mVisibleLimit (d) and mVisibleLimit * sqrt(2.0)
	// R^2 = d^2 + (d*sqrt(2.0))^2
	mVisibleLimit = radius * radius / 3.f;

	mPages.reserve(faceDivisions * faceDivisions);

	unsigned int pageId = 1;
	static float FACE = 5.f;
	static float CUBE_SIDE = 2.f * FACE;
	const float step = CUBE_SIDE / (float) faceDivisions;
	for (float d1 = -FACE; d1 < FACE; d1 += step) {
		for (float d2 = -FACE; d2 < FACE; d2 += step) {
			Log::debug("Planet page %s %.2f %.2f | step = %.2f", plane.c_str(), d1, d2, step);
			std::unique_ptr<PlanetPage> page = std::make_unique<PlanetPage>(faceId + pageId++, plane, radius, waterLevel, d1, d2, step, FACE, pageDivisions);

			if (d1 == -FACE && d2 == -FACE) {
				mCorners[0] = page->getCorner(0);
			}
			if (d1 == -FACE && (d2 + step) >= FACE) {
				mCorners[1] = page->getCorner(1);
			}
			if ((d1 + step) >= FACE && (d2 + step) >= FACE) {
				mCorners[2] = page->getCorner(2);
			}
			if ((d1 + step) >= FACE && d2 == -FACE) {
				mCorners[3] = page->getCorner(3);
			}

			mPages.push_back(std::move(page));
		}
	}
	setFieldOfView();
}


void PlanetFace::setFieldOfView() {
	mFOD = std::make_unique<FieldOfView>(PLANET_CENTER, mCorners[0], mCorners[1], mCorners[2], mCorners[3]);
}


void PlanetFace::initPhysics(btDynamicsWorld* dynamicsWorld) {
	for (auto& page : mPages)
		page->initPhysics(dynamicsWorld);
}


bool PlanetFace::isVisible(const ICamera* camera) const {
	if (mCameraVisibility.first == camera->getVersion())
		return mCameraVisibility.second;

	if (mFOD->isVisible(camera->getPosition())) {
		mCameraVisibility.first = camera->getVersion();
		mCameraVisibility.second = true;
		return true;
	}

	bool isVisible = false;
	const btVector3& innerPoint = mVisibleLimit * camera->getPosition().normalized();
	const btVector3& vInnerToCamera = camera->getPosition() - innerPoint;
	if (vInnerToCamera.dot(mCorners[0] - innerPoint) >= 0.0 ||
		vInnerToCamera.dot(mCorners[1] - innerPoint) >= 0.0 ||
		vInnerToCamera.dot(mCorners[2] - innerPoint) >= 0.0 ||
		vInnerToCamera.dot(mCorners[3] - innerPoint) >= 0.0)
	{
		// Here we check if at least one of the corners is visible to the camera
		isVisible = camera->isVisible(mCorners[0]) ||
					camera->isVisible(mCorners[1]) ||
					camera->isVisible(mCorners[2]) ||
					camera->isVisible(mCorners[3]);
	}
	mCameraVisibility.first = camera->getVersion();
	mCameraVisibility.second = isVisible;

	return isVisible;
}


void PlanetFace::getVisiblePageIds(const ICamera* camera, const btVector3& innerPoint, std::unordered_map<unsigned int,float>& pageIds, bool& hasVisibleWater) const {
	if (isVisible(camera)) {
		for (auto& page : mPages) {
			if (page->isVisible(camera, innerPoint)) {
				float distance = page->arcDistanceTo(camera->getPosition());
				pageIds[page->getPageId()] = distance;
				hasVisibleWater = hasVisibleWater || page->hasWater();
			}
		}
	}
}



void PlanetFace::enablePhysics(btDynamicsWorld *dynamicsWorld, const std::forward_list<std::shared_ptr<btRigidBody>>& rigidBodies) {
	for (auto& page : mPages) {
		page->enablePhysics(dynamicsWorld, rigidBodies);
	}
}


void PlanetFace::editVertices(const std::string& command, float value, const btVector3& point3D, float brushSize, const ICamera* camera, const btVector3& innerPoint) {
	if (isVisible(camera)) {
		for (auto& page : mPages)
			if (page->isVisible(camera, innerPoint))
				page->editVertices(command, value, point3D, brushSize);
	}
}


void PlanetFace::autoPaintVertices(const std::string &param, const btVector3 &point3D, float brushSize, const ICamera *camera, const btVector3& innerPoint) {
	if (isVisible(camera)) {
		for (auto& page : mPages)
			if (page->isVisible(camera, innerPoint))
				page->autoPaintVertices(param, point3D, brushSize);
	}
}


void PlanetFace::renderOpaque(const std::unordered_map<unsigned int,float>& visiblePages, const ICamera* camera, const GameState& gameState) {
	if (isVisible(camera)) {
		for (auto& page : mPages) {
			auto&& it = visiblePages.find(page->getPageId());
			if (it != visiblePages.end()) {
				page->renderOpaque(it->second, gameState);
			}
		}
	}
}


void PlanetFace::renderTranslucent(const std::unordered_map<unsigned int,float>& visiblePages, const ICamera *camera, const GameState &gameState) {
	if (isVisible(camera)) {
		for (auto& page : mPages) {
			auto&& it = visiblePages.find(page->getPageId());
			if (it != visiblePages.end()) {
				page->renderTranslucent(it->second, gameState);
			}
		}
	}
}


float PlanetFace::getHeightAt(const btVector3& direction) const {
	bool isVisible = mFOD->isVisible(direction);
	if (isVisible) {
		for (auto& page : mPages) {
			float h = page->getHeightAt(direction);
			if (h > 0.0f)
				return h;
		}
	}
	return -1.0f;
}


void PlanetFace::fixBorderNormals(const std::function<void(const btVector3&,btVector3&)>& getNormalAt, const btVector3& mouse3d, float brushSize, const ICamera* camera, const btVector3& innerPoint) {
	for (auto& page : mPages)
		page->fixBorderNormals(getNormalAt, mouse3d, brushSize, camera, innerPoint);
}


void PlanetFace::getNormalAt(const btVector3 &point, btVector3 &normal) const {
	for (const auto& page : mPages)
		page->getNormalAt(point, normal);
}


unsigned int PlanetFace::getPageId(const btVector3& point) const {
	for (const auto &page : mPages) {
		unsigned int pageId = page->getPageId(point);
		if (pageId > 0)
			return pageId;
	}
	return 0;
}


void PlanetFace::write(ISerializer* serializer) const {
	serializer->writeBegin(serializeID(), 0);

	serializer->write(mPages.size());
	serializer->write(mCorners[0]);
	serializer->write(mCorners[1]);
	serializer->write(mCorners[2]);
	serializer->write(mCorners[3]);

	for (const auto& page : mPages) {
		page->write(serializer);
	}
}

std::unique_ptr<PlanetFace> PlanetFace::create(ISerializer *serializer, std::weak_ptr<btDynamicsWorld> dynamicsWorld) {
	std::string className;
	unsigned long objectId;
	serializer->readBegin(className, objectId);
	if (className != serializeID() || objectId != 0)
		throw std::runtime_error("Not a planet face: " + className);

	unsigned long pageCount;
	serializer->read(pageCount);

	// calling private constructor, cannot call std::make_unique
	auto o = std::unique_ptr<PlanetFace>(new PlanetFace());
	serializer->read(o->mCorners[0]);
	serializer->read(o->mCorners[1]);
	serializer->read(o->mCorners[2]);
	serializer->read(o->mCorners[3]);

	for (unsigned int u = 0; u < pageCount; u++)
		o->mPages.push_back(PlanetPage::create(serializer, dynamicsWorld));

	o->setFieldOfView();
	return o;
}

std::string PlanetFace::serializeID() {
	return "PlanetFace";
}



