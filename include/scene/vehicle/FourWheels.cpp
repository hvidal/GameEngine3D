#include "FourWheels.h"
#include "../planet/Planet.h"

const std::string& FourWheels::SERIALIZE_ID = "FourWheels";

const static btVector3 WHEEL_DIR_CS0 = btVector3(0, -1, 0);
const static btVector3 WHEEL_AXLE_CS = btVector3(-1, 0, 0);

//-----------------------------------------------------------------------------

FourWheels::FourWheels(std::weak_ptr<btDynamicsWorld> dynamicsWorld, const btVector3& initialPosition, const std::string& carModelFilename, const std::string& wheelModelFilename):
	SceneObject(dynamicsWorld),
	mInitialPosition(initialPosition),
	mCarModelFilename(carModelFilename),
	mWheelModelFilename(wheelModelFilename),
	mWeight(500.f),
	mEngineForce(0.f),
	mBrakeForce(0.f),
	mRearGear(false),
	mMaxEngineForce(8500.f),
	mMaxBrakeForce(300.f),
	mVehicleSteering(0.22f),
	mSteeringIncrement(0.3f),
	mSteeringClamp(.7f),
	mWheelRadius(.65f),
	mWheelWidth(.35f),
	mWheelFriction(4.0f),
	mSuspensionStiffness(30.f),
	mSuspensionDamping(10.0f),
	mSuspensionCompression(0.0f),
	mSuspensionRestLength(0.1f),
	mRollInfluence(0.0f)
{
	mDynamicsWorld = dynamicsWorld;

	mCarModel = ModelOBJRendererCache::get(carModelFilename);
	mWheelModel = ModelOBJRendererCache::get(wheelModelFilename);

	mShapeRenderer = std::make_unique<GLShapeRenderer>();
}

FourWheels::~FourWheels() {
	Log::debug("Deleting FourWheels");
}


void FourWheels::updateSimulation() {
	// Front right
	mVehicle->setSteeringValue(mVehicleSteering, 0);
	mVehicle->applyEngineForce(mEngineForce, 0);

	// Front left
	mVehicle->setSteeringValue(mVehicleSteering, 1);
	mVehicle->applyEngineForce(mEngineForce, 1);

	// Brakes (rear wheels)
	mVehicle->setBrake(mBrakeForce, 2);
	mVehicle->setBrake(mBrakeForce, 3);

	const btVector3& centerOfMass = mVehicle->getRigidBody()->getCenterOfMassPosition();
	const btVector3& gravity = -centerOfMass.normalized() * 25.f; // points to the origin (planet center)
	mVehicle->getRigidBody()->setGravity(gravity);
}


void FourWheels::initPhysics(btTransform transform)
{
	float halfWidth = 1.2f;
	float halfHeight = 0.15f;
	float halfLength = 3.7f;

	float yShift = 0.2f;
	float zShift = -0.3f;

	buildVehicleShape(transform, halfWidth, halfHeight, halfLength, yShift, zShift);
	buildWheels(halfWidth, halfLength, yShift, zShift);
}


void FourWheels::buildVehicleShape(btTransform transform, float halfWidth, float halfHeight, float halfLength, float yShift, float zShift) {

	// Main shape
	auto puCompoundShape = std::make_unique<btCompoundShape>();

	auto _puChassisShape = std::make_unique<btBoxShape>(btVector3(halfWidth, halfHeight, halfLength));
	transform.setOrigin(btVector3(0, yShift, zShift)); // shifts the center of mass with respect to the chassis
	puCompoundShape->addChildShape(transform, _puChassisShape.get());

	// CABINET
	auto _puCabinetBottomShape = std::make_unique<btBoxShape>(btVector3(halfWidth, .25f, halfLength/2.f));
	transform.setOrigin(btVector3(0, yShift+halfHeight+.25f, zShift+halfLength/2.f));
	puCompoundShape->addChildShape(transform, _puCabinetBottomShape.get());
	//~
	auto _puCabinetGlassShape = CollisionShapeFactory::createAlignedTrapezium(
			.4f,
			halfWidth,
			1.4f,
			halfWidth,
			1.f);
	transform.setOrigin(btVector3(0, yShift+halfHeight+.6f+.5f, zShift+.6f));
	puCompoundShape->addChildShape(transform, _puCabinetGlassShape.get());

	// CARGO BOX
	auto _puCargoBoxLeftShape = std::make_unique<btBoxShape>(btVector3(.05f, .3f, halfLength/2.f - .05f));
	transform.setOrigin(btVector3(halfWidth - .05f, yShift+halfHeight+.3f, zShift - halfLength/2.f + .05f));
	puCompoundShape->addChildShape(transform, _puCargoBoxLeftShape.get());

	auto _puCargoBoxRightShape = std::make_unique<btBoxShape>(btVector3(.05f, .3f, halfLength/2.f - .05f));
	transform.setOrigin(btVector3(-halfWidth + .05f, yShift+halfHeight+.3f, zShift - halfLength/2.f + .05f));
	puCompoundShape->addChildShape(transform, _puCargoBoxRightShape.get());

	auto _puCargoBoxRearShape = std::make_unique<btBoxShape>(btVector3(halfWidth, .3f, .05f));
	transform.setOrigin(btVector3(0, yShift+halfHeight+.3f, zShift - halfLength + .05f));
	puCompoundShape->addChildShape(transform, _puCargoBoxRearShape.get());
	//

	transform.setIdentity();
	transform.setOrigin(mInitialPosition);
	createPhysicsBody(mWeight, transform, std::move(puCompoundShape));

	mPhysicsBody->addChildShape(std::move(_puChassisShape));
	mPhysicsBody->addChildShape(std::move(_puCabinetBottomShape));
	mPhysicsBody->addChildShape(std::move(_puCabinetGlassShape));
	mPhysicsBody->addChildShape(std::move(_puCargoBoxLeftShape));
	mPhysicsBody->addChildShape(std::move(_puCargoBoxRightShape));
	mPhysicsBody->addChildShape(std::move(_puCargoBoxRearShape));

	// never deactivate the vehicle
	mPhysicsBody->getRigidBody()->setActivationState(DISABLE_DEACTIVATION);

	mPhysicsBody->getRigidBody()->setDamping(0.0, 0.0);
	mPhysicsBody->getRigidBody()->setFriction(1.0f); // friction when the car touches the ground
	mPhysicsBody->getRigidBody()->setRestitution(0.25f); // how much of the kinetic energy remains after a collision

	// center of mass
	//transform.setIdentity();
	//transform.setOrigin(btVector3(0, 1.8f, 0.2f));
	//mRigidBody->setCenterOfMassTransform(transform);
}


void FourWheels::buildWheels(float halfWidth, float halfLength, float yShift, float zShift) {
	mWheelShape = std::make_unique<btCylinderShapeX>(btVector3(mWheelWidth, mWheelRadius, mWheelRadius));
	reset();

	auto psDynamicsWorld = mDynamicsWorld.lock();
	auto pDynamicsWorld = psDynamicsWorld.get();

	mVehicleRayCaster = std::make_unique<btDefaultVehicleRaycaster>(pDynamicsWorld);
	mVehicle = std::make_unique<btRaycastVehicle>(mTuning, mPhysicsBody->getRigidBody(), mVehicleRayCaster.get());
	pDynamicsWorld->addVehicle(mVehicle.get());

	float yConnection = yShift - mWheelRadius/2.f;
	float zConnection = zShift;

	//choose coordinate system
	int rightIndex = 0;
	int upIndex = 1;
	int forwardIndex = 2;
	mVehicle->setCoordinateSystem(rightIndex, upIndex, forwardIndex);

	// front right
	btVector3 connectionPointCS0(halfWidth + .1f, yConnection, zConnection + halfLength - mWheelRadius - .2f);
	addWheel(connectionPointCS0, true);

	// front left
	connectionPointCS0 = btVector3(-halfWidth - .1f, yConnection, zConnection + halfLength - mWheelRadius - .2f);
	addWheel(connectionPointCS0, true);

	// back right
	connectionPointCS0 = btVector3(halfWidth + .1f, yConnection, zConnection - halfLength + mWheelRadius + 1.1f);
	addWheel(connectionPointCS0, false);

	// back left
	connectionPointCS0 = btVector3(-halfWidth - .1f, yConnection, zConnection - halfLength + mWheelRadius + 1.1f);
	addWheel(connectionPointCS0, false);
}


void FourWheels::addWheel(const btVector3& connectionPointCS0, bool isFrontWheel)
{
	btWheelInfo& wheel = mVehicle->addWheel(
			connectionPointCS0,
			WHEEL_DIR_CS0,
			WHEEL_AXLE_CS,
			mSuspensionRestLength,
			mWheelRadius,
			mTuning,
			isFrontWheel);

	wheel.m_suspensionStiffness = mSuspensionStiffness;
	wheel.m_wheelsDampingRelaxation = mSuspensionDamping;
	wheel.m_wheelsDampingCompression = mSuspensionCompression;
	wheel.m_frictionSlip = mWheelFriction;
	wheel.m_rollInfluence = mRollInfluence;
}


void FourWheels::reset() {
	mVehicleSteering = 0.f;
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(mInitialPosition);

	const btVector3 origin(0,0,0);

	auto pRigidBody = mPhysicsBody->getRigidBody();
	//pRigidBody->setCenterOfMassTransform(transform);
	pRigidBody->setLinearVelocity(origin);
	pRigidBody->setAngularVelocity(origin);

	auto psDynamicsWorld = mDynamicsWorld.lock();
	psDynamicsWorld->getBroadphase()->getOverlappingPairCache()->cleanProxyFromPairs(mPhysicsBody->getRigidBody()->getBroadphaseHandle(), psDynamicsWorld->getDispatcher());

	if (mVehicle) {
		mVehicle->resetSuspension();
		for (int i = 0; i < mVehicle->getNumWheels(); i++) {
			//synchronize the wheels with the (interpolated) chassis worldtransform
			mVehicle->updateWheelTransform(i, true);
		}
	}
}


void FourWheels::turnLeft()
{
	mVehicleSteering += mSteeringIncrement;
	if (mVehicleSteering > mSteeringClamp)
		mVehicleSteering = mSteeringClamp;
}


void FourWheels::turnRight()
{
	mVehicleSteering -= mSteeringIncrement;
	if (mVehicleSteering < -mSteeringClamp)
		mVehicleSteering = -mSteeringClamp;
}


void FourWheels::moveForward()
{
	mEngineForce = mMaxEngineForce;
	mBrakeForce = 0.f;
	mRearGear = false;
}


void FourWheels::brake()
{
	if (!mRearGear) {
		mBrakeForce = mMaxBrakeForce;
		mEngineForce = 0.f;

		float linear = mPhysicsBody->getRigidBody()->getLinearVelocity().length();
		if (linear < 5.0)
			mRearGear = true;
	} else {
		mEngineForce = -mMaxEngineForce;
		mBrakeForce = 0.f;
	}
}


void FourWheels::jump() {
	btRigidBody* pRigidBody = mPhysicsBody->getRigidBody();
	btVector3 normal = pRigidBody->getCenterOfMassPosition().normalized();
	pRigidBody->setLinearVelocity(pRigidBody->getLinearVelocity() + 10.0f * normal);
	pRigidBody->setAngularVelocity(pRigidBody->getAngularVelocity() + btVector3(0.2f, 0.2f, 0.2f));
}


void FourWheels::render(bool opaque, const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) const {
	if (surfaceReflection && surfaceReflection->isRendering())
		return;

	static Matrix4x4 carMatrix4x4;
	carMatrix4x4 = getMatrix4x4();

	static std::vector<Matrix4x4> wheelMatrices(4);
	wheelMatrices.clear();

	const auto updateWheel = [this](int index){
		mVehicle->updateWheelTransform(index, true);

		Matrix4x4 wheelMatrix4x4 = getMatrix4x4(mVehicle->getWheelInfo(index).m_worldTransform);
		if (index % 2 == 1) {
			wheelMatrix4x4.rotate(0.0f, -1.0f, 0.f, 1.f, 0.f); // rotate 180 degrees
		}
		wheelMatrices.emplace_back(std::move(wheelMatrix4x4));
	};

	updateWheel(0);
	updateWheel(1);
	updateWheel(2);
	updateWheel(3);

	if (gameState.debugCode == DebugCode::COLLISION_SHAPE) {
		mShapeRenderer->render(camera, sky, mPhysicsBody->getCollisionShape(), carMatrix4x4, opaque);
		for (auto& wheelMatrix4x4 : wheelMatrices) {
			mShapeRenderer->render(camera, sky, mWheelShape.get(), wheelMatrix4x4, opaque);
		}
	} else {
		mCarModel->render(camera, sky, shadowMap, carMatrix4x4, opaque);
		mWheelModel->render(camera, sky, shadowMap, wheelMatrices, opaque);
	}
}


void FourWheels::renderOpaque(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) {
	render(true, camera, sky, shadowMap, surfaceReflection, gameState);
}


void FourWheels::renderTranslucent(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) {
	render(false, camera, sky, shadowMap, surfaceReflection, gameState);
}


void FourWheels::render2d(IRenderer2d* renderer2d) {
	int speed = static_cast<int>(mPhysicsBody->getRigidBody()->getLinearVelocity().length());
	const std::string text = "Speed " + std::to_string(speed) + " km/h";
	const static SDL_Color BLACK = { 0, 0, 0 };
	renderer2d->renderText(100, text, BLACK, "left=5", "top=10", 20);
}


void FourWheels::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mInitialPosition);
	serializer->write(mCarModelFilename);
	serializer->write(mWheelModelFilename);
	mPhysicsBody->write(serializer);
}


std::pair<std::string, Factory> FourWheels::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		btVector3 initialPosition;
		serializer->read(initialPosition);

		std::string carFilename, wheelFilename;
		serializer->read(carFilename);
		serializer->read(wheelFilename);

		IGameScene* gameScene = window->getGameScene().get();

		std::shared_ptr<FourWheels> o = std::make_shared<FourWheels>(gameScene->getDynamicsWorld(), initialPosition, carFilename, wheelFilename);
		o->setObjectId(objectId);
		o->initPhysics(btTransform::getIdentity());
		o->mPhysicsBody->read(serializer);

		gameScene->addSceneObject(o);

		// terrain-car contact
		std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(gameScene->getSerializable(Planet::SERIALIZE_ID));
		planet->interactWith(o->getSharedRigidBody());

		std::shared_ptr<KeyboardVehicleControl> keyboardArrowVehicleControl = std::make_shared<KeyboardVehicleControl>(o, SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_SPACE);
		gameScene->addKeyboardListener(keyboardArrowVehicleControl);

		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& FourWheels::serializeID() const noexcept {
	return SERIALIZE_ID;
}












