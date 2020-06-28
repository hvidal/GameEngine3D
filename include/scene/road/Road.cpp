#include <random>
#include "Road.h"
#include "../../util/ShadowMap.h"
#include "../../util/ContainerUtils.h"
#include "../../util/Texture.h"

static constexpr const char* vs[] = {
	"attribute vec4 position;"
	"attribute vec2 uv;"
	"attribute float border;"

	"uniform mat4 V;"
	"uniform mat4 P;"

	"varying vec3 worldV;"
	"varying vec2 _uv;"
	"varying float _border;"

	,ShadowMap::getVertexShaderCode(),

	"void main() {"
		"setShadowMap(position);"
		"worldV = position.xyz;"
		"_uv = uv;"
		"_border = border;"
		"gl_Position = P * V * position;"
	"}"
};

static constexpr const char* fs[] = {
	"uniform sampler2D material0;"
	"uniform float material0_scale;"

	,ShadowMap::getFragmentShaderCode(),
	"varying vec2 _uv;"
	"varying float _border;"
	"varying vec3 worldV;"

	"vec4 getPaint() {"
		"if (_border >= 0.93 && _border <= 0.96) return vec4(0.3,0.3,0.3,0.0);" // white line
		"if (_border > 0.929 && _border < 0.93 || _border > 0.96 && _border < 0.961) return vec4(0.1,0.1,0.1,0.0);" // white transition

		"if (_border >= 0.01 && _border <= 0.03) return vec4(0.3,0.3,0.05,0.0);" // yellow line
		"if (_border > 0.009 && _border < 0.01 || _border > 0.03 && _border < 0.031) return vec4(0.1,0.1,0.003,0.0);" // yellow transition
		"return vec4(0.0);"
	"}"

	"void main() {"
		"float shadow = getShadow9(worldV);"
		"vec4 c = texture2D(material0, _uv * material0_scale);"
		"c = c + getPaint();"
		"gl_FragColor = c * shadow;"
	"}"
};

static btVector3 gMouse3d;

const std::string& Road::SERIALIZE_ID = "Road";

//-----------------------------------------------------------------------------

/* private constructor */
Road::Road(std::shared_ptr<btDynamicsWorld> dynamicsWorld, std::weak_ptr<Planet> planet):
	SceneObject(dynamicsWorld),
	mPlanet(planet)
{
	mShader = std::make_unique<Shader>(vs, fs);
	mShader->bindAttribute(0, "position");
	mShader->bindAttribute(1, "uv");
	mShader->bindAttribute(2, "border");
	mShader->link();

	mPin = std::make_unique<Pin>();
}


Road::Road(std::shared_ptr<btDynamicsWorld> dynamicsWorld, std::weak_ptr<Planet> planet, std::unique_ptr<ITexture> texture, std::unique_ptr<ITexture> blockTexture):
	Road(dynamicsWorld, planet)
{
	mTexture = std::move(texture);
	mCityBlock = std::make_unique<CityBlock>(dynamicsWorld, planet, std::move(blockTexture));
}


Road::~Road() {
	Log::debug("Deleting Road");
	cleanup();
}


void Road::cleanup() {
	if (!mVertices.empty()) {
		mIndices.clear();
		mVertices.clear();
		glDeleteBuffers(1, &mVbo);
		glDeleteBuffers(1, &mIbo);
	}
}


void Road::initPhysics(btTransform transform) {
//	btVector3 v = btVector3(1,0,0).rotate(btVector3(0,1,0), 1.5707963267949f);
//	Log::debug("Rotate 90 degrees (1, 0, 0) = %.2f %.2f %.2f", v.x(), v.y(), v.z());

	// Todo fix error below
	//Height NOT found for (6013.51, 688.04, 250.56)
	//Height NOT found for (6025.44, 0.07, -0.00)
	//Height NOT found for (6006.54, 1.33, 613.84)
//	Log::debug("Height1 = %.2f", mPlanet->getHeightAt(btVector3(998.52, 0.00, 23.42).normalized()));

//	Log::debug("Height1 = %.2f", mPlanet->getHeightAt(btVector3(999.97, 0.00, 12.77)));
//	Log::debug("Height1 = %.2f", mPlanet->getHeightAt(btVector3(998.15f, 0.f, 13.39f)));
//	Log::debug("Height2 = %.2f", mPlanet->getHeightAt(btVector3(10.f, 140.f, -5.f)));
//	Log::debug("Height3 = %.2f", mPlanet->getHeightAt(btVector3(0.f, 0.32f, -5.f)));
//	Log::debug("Height4 = %.2f", mPlanet->getHeightAt(btVector3(-112.f, -.40f, 34.f)));
//	Log::debug("Height5 = %.2f", mPlanet->getHeightAt(btVector3(-310.f, 53.40f, -52.f)));
//	Log::debug("Height6 = %.2f", mPlanet->getHeightAt(btVector3(-13.3343f, -0.43432f, -52.323f)));
}


void Road::bind() {
	if (mVertices.empty())
		return;

	Log::debug("Binding Road %u %u | vertices=%u | indices=%u", mVbo, mIbo, mVertices.size(), mIndices.size());

	glGenBuffers(1, &mVbo);
	glGenBuffers(1, &mIbo);

	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, mVertices.size() * sizeof(RoadVertex), &mVertices[0].position, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndices.size() * sizeof(unsigned int), &mIndices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


void Road::renderOpaque(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const ISurfaceReflection* surfaceReflection, const GameState &gameState) {
	if (gameState.mode == GameMode::EDITING) {
		gMouse3d = gameState.mouse3d;
	}

	if (shadowMap->isRendering() || surfaceReflection->isRendering())
		return;

	if (mVertices.size() > 0) {
		IShader* pShader = mShader.get();

		pShader->run();
		shadowMap->setVars(pShader);
		camera->setMatrices(pShader);
		pShader->set("material0", mTexture.get());
		pShader->set("material0_scale", mTexture->getScaleFactor());

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, mVbo);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RoadVertex), 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(RoadVertex), (GLvoid*) offsetof(RoadVertex, uv));

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(RoadVertex), (GLvoid*) offsetof(RoadVertex, border));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(mIndices.size()), GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		IShader::stop();
	}

	mCityBlock->render(camera, sky, shadowMap, gameState);

	if (gameState.mode == GameMode::EDITING) {
		for (const auto& e : mPoints) {
			static const btVector3 SELECTED_COLOR = btVector3(.6f, 1.f, .4f);
			static const btVector3 SIMPLE_COLOR = btVector3(1.f, .6f, .4f);
			mPin->render(e.second, camera, sky->getSunPosition(), e.first == mSelectedPointId? SELECTED_COLOR : SIMPLE_COLOR);
		}
		for (const Block& block : mBlocks) {
			for (unsigned int id : block.pointIds) {
				const btVector3& p = mPoints[id];
				const btVector3& p1 = p.normalized();
				mPin->render(p + p1, camera, sky->getSunPosition(), btVector3(.1f, .1f, .8f));
			}
		}
/*
		glLineWidth(8.0);
		glColor3f(0.0, 0.0, 1.0);
		glBegin(GL_LINES);
		for (const RoadStretch& s : mRoadStretchList) {
			const btVector3 &a = mPoints[s.p0];
			const btVector3 &b = mPoints[s.p1];
			glVertex3f(a.x(), a.y(), a.z());
			glVertex3f(b.x(), b.y(), b.z());
		}
		glEnd();
*/
	}
}


std::default_random_engine GENERATOR;
std::uniform_int_distribution<unsigned int> DISTRIBUTION(1,99999999);

unsigned int newId(std::unordered_map<unsigned int, btVector3>& points) {
	unsigned int id = DISTRIBUTION(GENERATOR);
	while (std::find_if(points.cbegin(), points.cend(), [&id](const auto& e){ return e.first == id; }) != points.cend()) {
		id = DISTRIBUTION(GENERATOR);
	}
	return id;
}

unsigned int addPoint(const btVector3& p, std::unordered_map<unsigned int, btVector3>& points) {
	unsigned int id = newId(points);
	points[id] = p;
	return id;
}

constexpr const static float FIND_POINT_DISTANCE = 10.f;

unsigned int findPointAt(const btVector3& p, const std::unordered_map<unsigned int, btVector3>& points) {
	const auto& it = std::find_if(points.cbegin(), points.cend(), [&p](const auto& e){ return e.second.distance(p) < FIND_POINT_DISTANCE; });
	return it == points.cend()? 0 : it->first;
}

static std::vector<unsigned int> gBlockPoints;

ISceneObject::CommandMap Road::getCommands() {
	ISceneObject::CommandMap map;

	map["road0"] = [this](const std::string& param) {
		unsigned int pId = findPointAt(gMouse3d, mPoints);
		if (pId == 0) {
			mSelectedPointId = newId(mPoints);
			mPoints[mSelectedPointId] = mPlanet.lock()->getSurfacePoint(gMouse3d);
			Log::debug("Starting new road at (%.2f, %.2f, %.2f)", gMouse3d.x(), gMouse3d.y(), gMouse3d.z());
		} else {
			mSelectedPointId = pId;
			Log::debug("Selecting road at (%.2f, %.2f, %.2f)", gMouse3d.x(), gMouse3d.y(), gMouse3d.z());
		}
	};

	map["road1"] = [this](const std::string& param) {
		unsigned int pId = findPointAt(gMouse3d, mPoints);
		RoadStretch s;
		s.p0 = mSelectedPointId;
		s.p1 = pId == 0? addPoint(mPlanet.lock()->getSurfacePoint(gMouse3d), mPoints) : pId;
		mRoadStretchList.push_front(s);
		Log::debug("Connecting road at (%.2f, %.2f, %.2f)", gMouse3d.x(), gMouse3d.y(), gMouse3d.z());
		mSelectedPointId = s.p1;
	};

	map["selectroad"] = [this](const std::string& param) {
		unsigned int pId = findPointAt(gMouse3d, mPoints);
		if (pId > 0)
			mSelectedPointId = pId;
	};

	map["moveroad"] = [this](const std::string& param) {
		if (mSelectedPointId > 0) {
			mPoints[mSelectedPointId] = gMouse3d;
		}
	};

	map["buildroad"] = [this](const std::string& param) {
		buildRoad();
	};

	map["block0"] = [this](const std::string& param) {
		gBlockPoints.clear();

		unsigned int pId = findPointAt(gMouse3d, mPoints);
		if (pId > 0) {
			gBlockPoints.push_back(pId);
			Log::debug("Starting block with point ID = %d", pId);
		}
	};

	map["block1"] = [this](const std::string& param) {
		unsigned int pId = findPointAt(gMouse3d, mPoints);
		if (pId > 0) {
			// if this point equals the first point, then the block is complete
			if (pId == gBlockPoints[0]) {
				Log::debug("Block finished with %d points", gBlockPoints.size());
				Block block;
				block.pointIds.assign(gBlockPoints.cbegin(), gBlockPoints.cend());
				block.isOpen = param == "open";
				mBlocks.push_front(block);
				gBlockPoints.clear();
			} else {
				// Todo check CCW rotation
				gBlockPoints.push_back(pId);
				Log::debug("Point %d added to current block", pId);
			}
		}
	};

	return map;
}


constexpr const static float ROAD_HALF_WIDTH = 10.f;
constexpr const static float ROAD_JUNCTION_RADIUS = 15.f;
constexpr const static float ROAD_SLICE_SIZE = 4.f;
constexpr const static float CCW_90_DEGREES = 1.5707963267949f; // positive: counter clockwise rotation
constexpr const static int CURVE_SECTIONS = 20;

/**	Find the min angle between two direction vectors in a curve point.
	When the angle is less than the min angle, then the mesh of the streets will have to be adjusted.
	The min angle happens when two streets touch each other close to the junction point
	So half of the angle has half of the road width on the opposite side, and the junction radius is the adjacent side.
	tan(half_angle) = half_road_width / junction_radius */
static const float CURVE_MIN_ANGLE = (float) (2.f * atan(ROAD_HALF_WIDTH / ROAD_JUNCTION_RADIUS));


void Road::buildRoad() {
	cleanup();

	std::vector<std::shared_ptr<JunctionMesh>> junctionMeshes;
	std::vector<std::shared_ptr<RoadMesh>> roadMeshes;

	// Utility function
	const static std::function<std::shared_ptr<RoadMesh>(const RoadStretch&)> getRoadMesh = [&roadMeshes](const RoadStretch& stretch) {
		const auto &roadMeshIterator = std::find_if(roadMeshes.begin(), roadMeshes.end(),
			[stretch](auto& mesh) {
				return
					mesh->stretch.p0 == stretch.p0 &&
					mesh->stretch.p1 == stretch.p1;
			});
		std::shared_ptr<RoadMesh> roadMesh;
		if (roadMeshIterator == roadMeshes.end()) {
			roadMesh = std::make_shared<RoadMesh>();
			roadMesh->stretch = stretch;
			roadMeshes.push_back(roadMesh);
		} else {
			roadMesh = *roadMeshIterator;
		}
		return roadMesh;
	};

	// Utility function
	const std::function<bool(RoadMesh*,unsigned int,bool)> updateRoadMesh = [this](RoadMesh* roadMesh, unsigned int pointId, bool isDeadEnd){
		bool isStart = roadMesh->stretch.p0 == pointId;

		btVector3 point = mPoints[pointId];
		const btVector3& up = point.normalized();
		const btVector3& direction = (mPoints[roadMesh->stretch.p1] - mPoints[roadMesh->stretch.p0]).normalized();
		const btVector3& rotated = ROAD_HALF_WIDTH * direction.rotate(up, CCW_90_DEGREES);
		if (!isDeadEnd)
			point += ROAD_JUNCTION_RADIUS * (isStart? direction : -direction);

		std::shared_ptr<Planet> planet = mPlanet.lock();
		btVector3* d = isStart? &roadMesh->start[0] : &roadMesh->end[0];
		d[0] = planet->getSurfacePoint(point + rotated);
		d[1] = planet->getSurfacePoint(point);
		d[2] = planet->getSurfacePoint(point - rotated);

		return isStart;
	};

	// Utility function
	const static std::function<void(std::vector<unsigned int>&,unsigned int)> buildIndices = [](std::vector<unsigned int>& indices, unsigned int start){
		indices.push_back(start + 0);
		indices.push_back(start + 1);
		indices.push_back(start + 3);

		indices.push_back(start + 1);
		indices.push_back(start + 4);
		indices.push_back(start + 3);

		indices.push_back(start + 1);
		indices.push_back(start + 2);
		indices.push_back(start + 4);

		indices.push_back(start + 2);
		indices.push_back(start + 5);
		indices.push_back(start + 4);
	};

	// Utility function
	const static auto setLastBorderWeights = [](std::vector<btVector3>& v){
		unsigned long size = v.size();
		v[size-1].setW(1.f);
		v[size-2].setW(0.f);
		v[size-3].setW(1.f);
	};

	std::shared_ptr<Planet> planet = mPlanet.lock();
	Planet* pPlanet = planet.get();

	// For each junction, find the best way to connect the streets
	for (const auto& it : mPoints) {
		const unsigned int pointId = it.first;
		std::vector<RoadStretch> stretches;
		stretches.reserve(4);
		// First we collect all stretches that connect to this point
		std::for_each(mRoadStretchList.cbegin(), mRoadStretchList.cend(), [&pointId, &stretches](const RoadStretch& stretch) {
			if (stretch.p0 == pointId || stretch.p1 == pointId)
				stretches.push_back(stretch);
		});

		std::shared_ptr<JunctionMesh> junctionMesh = std::make_shared<JunctionMesh>();
		junctionMesh->stretches.assign(stretches.cbegin(), stretches.cend());
		junctionMeshes.push_back(junctionMesh);
		junctionMesh->pointId = pointId;

		if (stretches.size() == 1) {
			const RoadStretch& stretch = stretches[0];

			auto roadMesh = getRoadMesh(stretch);
			bool isStart = updateRoadMesh(roadMesh.get(), pointId, true);
			btVector3* points = isStart? &roadMesh->start[0] : &roadMesh->end[0];

			btVector3 middlePoint = points[1];
			btVector3 outerPoint = isStart? points[0] : points[2]; // CCW rotation, find the first outer point
			const btVector3& up = middlePoint.normalized();
			btVector3 rotation = outerPoint - middlePoint;

			const static float rotationAngle = 2.f * CCW_90_DEGREES / CURVE_SECTIONS;

			junctionMesh->vertices.reserve(8);
			junctionMesh->indices.reserve(3 * CURVE_SECTIONS); // 3 vertices * CURVE_SECTIONS triangles

			middlePoint.setW(0.f);
			outerPoint.setW(1.f);

			junctionMesh->vertices.push_back(middlePoint);
			junctionMesh->vertices.push_back(outerPoint);
			for (unsigned int i = 0; i < CURVE_SECTIONS; i++) {
				rotation = rotation.rotate(up, rotationAngle);
				btVector3 p = pPlanet->getSurfacePoint(middlePoint + rotation);
				p.setW(1.f);
				junctionMesh->vertices.push_back(p);

				junctionMesh->indices.push_back(0);
				junctionMesh->indices.push_back(i + 1);
				junctionMesh->indices.push_back(i + 2);
			}
		} else if (stretches.size() == 2) {
			const RoadStretch& stretch0 = stretches[0];
			const RoadStretch& stretch1 = stretches[1];
			auto roadMesh0 = getRoadMesh(stretch0);
			auto roadMesh1 = getRoadMesh(stretch1);

			const btVector3& point = mPoints[pointId];
			const btVector3& up = point.normalized();

			bool isStart0 = updateRoadMesh(roadMesh0.get(), pointId, false);
			bool isStart1 = updateRoadMesh(roadMesh1.get(), pointId, false);

			btVector3 direction0 = (mPoints[stretch0.p1] - mPoints[stretch0.p0]).normalized();
			btVector3 direction1 = (mPoints[stretch1.p1] - mPoints[stretch1.p0]).normalized();

			direction0 = isStart0? direction0 : -direction0;
			direction1 = isStart1? direction1 : -direction1;

			// Fix the direction vectors: make sure they are tangent to the planet's surface
			btVector3 p0 = pPlanet->getSurfacePoint(point + direction0);
			btVector3 p1 = pPlanet->getSurfacePoint(point + direction1);
			direction0 = (p0 - point).normalized();
			direction1 = (p1 - point).normalized();

			float angle = direction0.angle(direction1);

			// The sum of both directions will point to the inner side of the curve
			// But we have to make sure that it is tangent to the surface of the planet
			// (instead of pointing down or up)
			btVector3 curveInner = direction0 + direction1;
			btVector3 toTheSide = curveInner.cross(up);
			const btVector3& curve1 = up.cross(toTheSide).normalized();

			if (angle < CURVE_MIN_ANGLE) {
				Log::info("Angle between roads is too small, NOT IMPLEMENTED.");
			} else {
				// The center of the curve can be calculated with trigonometry
				// Vector curve1 cuts the angle in half. The distance to the center of the curve
				// is the hypotenuse. The direction vector multiplied by ROAD_JUNCTION_RADIUS will be
				// the adjacent side.
				// cos(angle/2) = ROAD_JUNCTION_RADIUS / distance
				float halfAngle = angle / 2.f;
				float distance = (float) (ROAD_JUNCTION_RADIUS / cos(halfAngle));
				btVector3 curveCenter = point + distance * curve1;

				const btVector3* points0 = isStart0? &roadMesh0->start[0] : &roadMesh0->end[0];
				const btVector3* points1 = isStart1? &roadMesh1->start[0] : &roadMesh1->end[0];

				// Now we have to learn more about the curve
				const btVector3& c0 = points0[1] - curveCenter;
				const btVector3& c1 = points1[1] - curveCenter;
				float centerAngle = c0.angle(c1);
				// We have to check if the curve is to the left or right
				// The cross product will be upwards if the c1 is to the left of c0
				btVector3 cross = c0.cross(c1);
				bool isLeftTurn = cross.dot(up) >= 0.f;

				// We always rotate from points0 to points1
				const btVector3* start = points0;
				const btVector3* end = points1;

				// Create three vectors that will be rotated in each step of the loop
				// The left/right vertices change if the stretch is a start or end
				int first = isStart0? 2 : 0;
				int last = isStart0? 0 : 2;
				const btVector3& v0 = start[first] - curveCenter;
				const btVector3& v1 = start[1] - curveCenter;
				const btVector3& v2 = start[last] - curveCenter;

				junctionMesh->vertices.reserve(3 * CURVE_SECTIONS);
				junctionMesh->indices.reserve(3 * 4 * CURVE_SECTIONS); // 3 vertices * 4 triangles * sections

				float step = centerAngle / (float) CURVE_SECTIONS;
				for (unsigned int i = 0; i < CURVE_SECTIONS; ++i) {
					// the step angle must be negative if we are rotating clockwise
					float currentAngle = i * (isLeftTurn? step : -step);
					btVector3 _v0 = pPlanet->getSurfacePoint(curveCenter + v0.rotate(up, currentAngle));
					btVector3 _v1 = pPlanet->getSurfacePoint(curveCenter + v1.rotate(up, currentAngle));
					btVector3 _v2 = pPlanet->getSurfacePoint(curveCenter + v2.rotate(up, currentAngle));

					junctionMesh->vertices.push_back(_v0);
					junctionMesh->vertices.push_back(_v1);
					junctionMesh->vertices.push_back(_v2);
					setLastBorderWeights(junctionMesh->vertices);

					buildIndices(junctionMesh->indices, 3 * i);
				}
				// Last vertices should match the end line
				int first1 = isStart1? 0 : 2;
				int last1 = isStart1? 2 : 0;

				btVector3 _v0 = pPlanet->getSurfacePoint(end[first1]);
				btVector3 _v1 = pPlanet->getSurfacePoint(end[1]);
				btVector3 _v2 = pPlanet->getSurfacePoint(end[last1]);

				junctionMesh->vertices.push_back(_v0);
				junctionMesh->vertices.push_back(_v1);
				junctionMesh->vertices.push_back(_v2);
				setLastBorderWeights(junctionMesh->vertices);
			}

		} else if (stretches.size() >= 3) {

			const auto findPointsCCW = [this, &updateRoadMesh, &pPlanet](std::vector<btVector3>& points, unsigned int pointId, std::vector<std::shared_ptr<RoadMesh>>& meshes){

				const unsigned long count = meshes.size();
				const btVector3* connPoints[count];
				const unsigned int* order[count];

				const static unsigned int START[] = {2,1,0};
				const static unsigned int END[] = {0,1,2};

				int i = 0;
				for (auto& mesh : meshes) {
					bool isStart = updateRoadMesh(mesh.get(), pointId, false);
					connPoints[i] = isStart? &mesh->start[0] : &mesh->end[0];
					order[i] = isStart? &START[0] : &END[0];
					i++;
				}

				auto getPoint = [&connPoints, &order](int _index, int _order) {
					const btVector3* p = connPoints[_index];
					const unsigned int* o = order[_index];
					return p[o[_order]];
				};

				const btVector3& point = mPoints[pointId];
				const btVector3& up = point.normalized();

				points.push_back(point); // first, index 0

				// add mesh0 first, points are in CCW order (left to right)
				points.push_back(getPoint(0,0));
				points.push_back(getPoint(0,1));
				points.push_back(getPoint(0,2));
				setLastBorderWeights(points);

				// Compare mesh0 with other meshes
				const btVector3& v0 = getPoint(0,1) - point;
				// for each index, we have to store the angle it makes with mesh0
				std::vector<std::pair<int,float>> angles;
				const static float TWO_PI = 6.28318530718f;

				for (int k = 1; k < meshes.size(); k++) {
					const btVector3& vk = getPoint(k,1) - point;
					float angle0k = v0.angle(vk);
					// Use the cross product to see if the current angle should be adjusted to CCW
					// If the cross vector points down, then the angle is actually (360 - angle)
					const btVector3& cross0k = v0.cross(vk);
					if (cross0k.dot(up) < 0.f)
						angle0k = TWO_PI - angle0k;

					angles.push_back(std::make_pair(k, angle0k));
				}

				auto addIntermediatePoints = [&point, &points, &pPlanet](const btVector3& a, const btVector3& b){
					const static float stepLength = 2.f;
					const btVector3& direction = b - a;
					float length = direction.length();
					if (length < stepLength)
						return;

					const static float tolerance = 1.f; // small tolerance to avoid narrow triangles
					const btVector3& dir1 = direction.normalized();
					float curr = stepLength;
					while (curr < length - tolerance) {
						btVector3 p = a + curr * dir1;
						if (p.distance(point) < ROAD_JUNCTION_RADIUS) {
							const btVector3& d1 = (p - point).normalized();
							p = point + ROAD_JUNCTION_RADIUS * d1;
						}
						p = pPlanet->getSurfacePoint(p);
						p.setW(1.f);
						points.push_back(p);
						curr += stepLength;
					}
				};

				// Sort the angles so that we can go through them in CCW order
				std::sort(angles.begin(), angles.end(), [](std::pair<int,float>& a, std::pair<int,float>& b){ return a.second < b.second; });

				btVector3 currentPoint = getPoint(0,2);
				for (const std::pair<int,float>& pair : angles) {
					int index = pair.first;
					addIntermediatePoints(currentPoint, getPoint(index,0));
					points.push_back(getPoint(index,0));
					points.push_back(getPoint(index,1));
					points.push_back(getPoint(index,2));
					setLastBorderWeights(points);

					currentPoint = getPoint(index, 2);
				}
				addIntermediatePoints(currentPoint, getPoint(0,0));
			};

			std::vector<std::shared_ptr<RoadMesh>> meshes;
			for (const RoadStretch& s : stretches) {
				auto mesh = getRoadMesh(s);
				meshes.push_back(mesh);
			}

			std::vector<btVector3> ccwPoints;
			ccwPoints.reserve(100);
			findPointsCCW(ccwPoints, pointId, meshes);

			const static auto triangulate = [this, pPlanet](std::vector<btVector3>& points, JunctionMesh* mesh) {
				unsigned long numPoints = points.size();
				mesh->vertices.reserve(2 * (numPoints-1) + 1); // see loop below
				mesh->indices.reserve(9 * (numPoints-1)); // see loop below

				btVector3 centerPoint = points[0];
				centerPoint.setW(0.f);
				mesh->vertices.push_back(centerPoint); // center point

				unsigned int size = (unsigned int) points.size();
				for (unsigned int i = 1; i < size; i++) {
					mesh->vertices.push_back(points[i]);
					// Create a middle point
					btVector3 v = .5f * (centerPoint - points[i]);
					btVector3 middle = pPlanet->getSurfacePoint(points[i] + v);
					middle.setW(.2f); // special weight / will make the center circle bigger
					mesh->vertices.push_back(middle);

					unsigned int s = (2 * i) - 1;
					unsigned int next = i == size - 1? 1 : s + 2;

					// If this is the middle of the road (w = 0)
					// then we have to revert the triangle in order to keep the
					// white border line smooth around the edges of the road
					if (points[i].w() == 0.f) {
						mesh->indices.push_back(s);
						mesh->indices.push_back(next+1);
						mesh->indices.push_back(s+1);

						mesh->indices.push_back(s);
						mesh->indices.push_back(next);
						mesh->indices.push_back(next+1);
					} else {
						mesh->indices.push_back(s);
						mesh->indices.push_back(next);
						mesh->indices.push_back(s+1);

						mesh->indices.push_back(next);
						mesh->indices.push_back(next+1);
						mesh->indices.push_back(s+1);
					}

					mesh->indices.push_back(s+1);
					mesh->indices.push_back(next+1);
					mesh->indices.push_back(0); // center point
				}
			};

			triangulate(ccwPoints, junctionMesh.get());
		}
	}

	// --- Build roads (straight sections)

	for (auto& mesh : roadMeshes) {
		float length = mesh->start[1].distance(mesh->end[1]);
		unsigned int slices = (unsigned int) floor(length / ROAD_SLICE_SIZE);
		float sliceLength = length / (float) slices;

		unsigned int vertexCount = 3 * (slices + 1);
		unsigned int triangleCount = 4 * slices;
		mesh->vertices.reserve(vertexCount);
		mesh->indices.reserve(3 * triangleCount);

		const btVector3& direction = (mPoints[mesh->stretch.p1] - mPoints[mesh->stretch.p0]).normalized();
		const btVector3& step = sliceLength * direction;

		// Build the vertices around each slice
		// It is basically three points for each section: left, middle, right
		for (unsigned int slice = 0; slice < slices; slice++) {
			const btVector3& _step = slice * step;
			const btVector3& a = mesh->start[0] + _step;
			const btVector3& b = mesh->start[1] + _step;
			const btVector3& c = mesh->start[2] + _step;

			mesh->vertices.push_back(pPlanet->getSurfacePoint(a));
			mesh->vertices.push_back(pPlanet->getSurfacePoint(b));
			mesh->vertices.push_back(pPlanet->getSurfacePoint(c));
			setLastBorderWeights(mesh->vertices);

			// Four triangles per slice
			unsigned int s = 3 * slice;
			buildIndices(mesh->indices, s);
		}

		// The last three vertices should meet the end point
		mesh->vertices.push_back(pPlanet->getSurfacePoint(mesh->end[0]));
		mesh->vertices.push_back(pPlanet->getSurfacePoint(mesh->end[1]));
		mesh->vertices.push_back(pPlanet->getSurfacePoint(mesh->end[2]));
		setLastBorderWeights(mesh->vertices);
	}

	buildCityBlocks(junctionMeshes, roadMeshes);
	buildFinalMesh(junctionMeshes, roadMeshes);
}


void Road::buildCityBlocks(const std::vector<std::shared_ptr<JunctionMesh>>& junctionMeshes, const std::vector<std::shared_ptr<RoadMesh>>& roadMeshes)
{
	const std::function<void(unsigned int, unsigned int,std::vector<btVector3>&)> &findLeftRoadMeshPoints = [this, &roadMeshes](unsigned int p0, unsigned int p1, std::vector<btVector3>& points) {
		const btVector3 &v0 = mPoints[p0];
		const btVector3 &v1 = mPoints[p1];

		for (const auto& mesh : roadMeshes) {
			const RoadStretch& s = mesh->stretch;
			bool hasP0 = s.p0 == p0 || s.p1 == p0;
			bool hasP1 = s.p0 == p1 || s.p1 == p1;
			if (hasP0 && hasP1) {
				bool isStart = p0 == s.p0;
				if (isStart) {
					long pos = 0;
					while (pos < mesh->vertices.size()) {
						points.push_back(mesh->vertices[pos]);
						pos += 3;
					}
				} else {
					long pos = mesh->vertices.size() - 1;
					while (pos >= 0) {
						points.push_back(mesh->vertices[pos]);
						pos -= 3;
					}
				}
			}
		}
	};

	const std::function<void(unsigned int, unsigned int,std::vector<btVector3>&)> &findLeftJunctionMeshPoints = [this, &junctionMeshes](unsigned int p0, unsigned int p1, std::vector<btVector3>& points) {
		for (const auto& mesh : junctionMeshes) {
			if (mesh->pointId == p1) {
				// We have to collect points only when there are two stretches.
				// Other cases are irrelevant because connections are straight lines.
				if (mesh->stretches.size() == 2) {
					// Find the stretch that contains p1 (it is the stretch that doesn't contain p0)
					const RoadStretch& stretch1 = mesh->stretches[0].p0 == p0 || mesh->stretches[0].p1 == p0? mesh->stretches[1] : mesh->stretches[0];
					unsigned int p2 = stretch1.p0 == p1? stretch1.p1 : stretch1.p0;

					const btVector3& dir1 = mPoints[p1] - mPoints[p0];
					const btVector3& dir2 = mPoints[p2] - mPoints[p1];
					const btVector3& up = mPoints[p1].normalized();

					// Find the normal vectors that point to the left side of the curve
					// Use the right hand rule (index finger up, middle finger is the direction, thumb is the normal vector)
					const btVector3& normal1 = up.cross(dir1);
					const btVector3& normal2 = up.cross(dir2);

					const btVector3& vp1 = mPoints[p1];
					std::vector<btVector3> temp;
					for (const btVector3& v : mesh->vertices) {
						if (v.w() == 1.f) {
							bool ok1 = (v - vp1).dot(normal1) >= 0;
							bool ok2 = (v - vp1).dot(normal2) >= 0;
							if (ok1 && ok2) {
								temp.push_back(v);
							}
						}
					}
					// Now we have to check if the order of the points is correct
					// We have to check if they rotate counter clockwise
					// let's use the right hand rule
					const btVector3& a = vp1 - temp[0];
					const btVector3& b = temp[1] - temp[0];
					const btVector3& cross = a.cross(b);
					bool isLeftCurve = cross.dot(up) >= 0;
					if (isLeftCurve) {
						// normal order
						std::for_each(temp.cbegin(), temp.cend(), [&points](const btVector3& v){ points.push_back(v); });
					} else {
						// reverse order
						std::for_each(temp.crbegin(), temp.crend(), [&points](const btVector3& v){ points.push_back(v); });
					}
				}
			}
		}
	};

	mCityBlock->cleanup();

	std::shared_ptr<Planet> planet = mPlanet.lock();

	for (const Block& block : mBlocks) {
		std::vector<btVector3> ccwPoints;
		btVector3 blockCenter(0.f, 0.f, 0.f);
		unsigned long size = block.pointIds.size();
		for (int i = 0; i < size; i++) {
			int next = static_cast<int>((i + 1) % size);
			unsigned int p0 = block.pointIds[i];
			unsigned int p1 = block.pointIds[next];
			findLeftRoadMeshPoints(p0, p1, ccwPoints);
			findLeftJunctionMeshPoints(p0, p1, ccwPoints);
			blockCenter = blockCenter + mPoints[p0];
		}
		mCityBlock->add(planet->getSurfacePoint(blockCenter), ccwPoints, block.isOpen);
	}
	mCityBlock->bind();
	mCityBlock->initPhysics();
}


void Road::buildFinalMesh(const std::vector<std::shared_ptr<JunctionMesh>>& junctionMeshes, const std::vector<std::shared_ptr<RoadMesh>>& roadMeshes) {
	// Merge all meshes
	unsigned int vertexCount = 0;
	unsigned int indiceCount = 0;
	for (auto& mesh : junctionMeshes) {
		vertexCount += mesh->vertices.size();
		indiceCount += mesh->indices.size();
	}
	for (auto& mesh : roadMeshes) {
		vertexCount += mesh->vertices.size();
		indiceCount += mesh->indices.size();
	}

	mVertices.reserve(vertexCount);
	mIndices.reserve(indiceCount);

	for (auto& mesh : junctionMeshes) {
		unsigned int pos = static_cast<unsigned int>(mVertices.size());
		for (const btVector3& p : mesh->vertices) {
			RoadVertex vertex;
			vertex.position = p;
			vertex.border = p.w();
			Planet::convertPointToUV(vertex.position, &vertex.uv[0]);
			mVertices.push_back(vertex);
		}
		for (unsigned int i : mesh->indices)
			mIndices.push_back(pos + i);
	}

	for (auto& mesh : roadMeshes) {
		unsigned int pos = static_cast<unsigned int>(mVertices.size());
		for (const btVector3& p : mesh->vertices) {
			RoadVertex vertex;
			vertex.position = p;
			vertex.border = p.w();
			Planet::convertPointToUV(vertex.position, &vertex.uv[0]);
			mVertices.push_back(vertex);
		}
		for (unsigned int i : mesh->indices)
			mIndices.push_back(pos + i);
	}

	bind();
	Log::debug("[Finished Road] Vertices = %d | Indices = %d", mVertices.size(), indiceCount);
}


void Road::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());

	serializer->write(mPoints.size());
	for (const auto& e : mPoints) {
		serializer->write(e.first);
		serializer->write(e.second);
	}

	serializer->write(ContainerUtils::count(mRoadStretchList));
	for (const RoadStretch& s : mRoadStretchList) {
		serializer->write(s.p0);
		serializer->write(s.p1);
	}

	serializer->write(ContainerUtils::count(mBlocks));
	for (const Block& b : mBlocks) {
		b.write(serializer);
	}

	serializer->write(mVertices.size());
	for (const RoadVertex& v : mVertices) {
		v.write(serializer);
	}
	serializer->write(mIndices);

	mTexture->write(serializer);

	mCityBlock->write(serializer);
}


std::pair<std::string, Factory> Road::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window){
		std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(window->getGameScene()->getSerializable(Planet::SERIALIZE_ID));
		std::shared_ptr<btDynamicsWorld> dynamicsWorld = window->getGameScene()->getDynamicsWorld();

		// calling private constructor, cannot call std::make_shared
		std::shared_ptr<Road> o = std::shared_ptr<Road>(new Road(dynamicsWorld, planet));
		o->setObjectId(objectId);

		std::size_t pointCount, stretchCount, blockCount, vertexCount;

		serializer->read(pointCount);
		o->mPoints.reserve(pointCount);
		for (int i = 0; i < pointCount; i++) {
			unsigned int id;
			serializer->read(id);
			serializer->read(o->mPoints[id]);
		}

		serializer->read(stretchCount);
		for (int i = 0; i < stretchCount; i++) {
			RoadStretch s;
			serializer->read(s.p0);
			serializer->read(s.p1);
			o->mRoadStretchList.push_front(s);
		}

		serializer->read(blockCount);
		for (int i = 0; i < blockCount; i++) {
			o->mBlocks.push_front(Block::read(serializer));
		}

		serializer->read(vertexCount);
		o->mVertices.reserve(vertexCount);
		for (int i = 0; i < vertexCount; i++) {
			o->mVertices.push_back(RoadVertex::read(serializer));
		}
		serializer->read(o->mIndices);

		auto textureFactory = serializer->getFactory(Texture::SERIALIZE_ID);
		std::string name;
		unsigned long oId;
		serializer->readBegin(name, oId);
		std::shared_ptr<ISerializable> tex = textureFactory.create(oId, serializer, window);
		o->mTexture = std::dynamic_pointer_cast<Texture>(tex);
		o->mTexture->mipmap()->repeat();

		serializer->readBegin(name, oId);
		o->mCityBlock = CityBlock::read(serializer, window, dynamicsWorld, planet);

		o->bind();

		window->getGameScene()->addSceneObject(o);
		o->initPhysics(btTransform::getIdentity());

		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& Road::serializeID() const noexcept {
	return SERIALIZE_ID;
}

