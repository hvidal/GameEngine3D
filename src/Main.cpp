#include <stdexcept>
#include <string>

#if defined(_MSC_VER)
#include <SDL.h>
#elif defined(__clang__)
#include <SDL2/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "../include/app/Interfaces.h"
#include "../include/app/Window.h"
#include "../include/app/Application.h"
#include "../include/app/Serializer.h"
#include "../include/scene/planet/Planet.h"
#include "../include/scene/vehicle/FourWheels.h"
#include "../include/scene/road/Road.h"
#include "../include/scene/Renderer2d.h"
#include "../include/scene/sky/Clouds.h"
#include "../include/scene/sky/Sky.h"
#include "../include/camera/WASDCamera.h"
#include "../include/camera/ChaseCamera.h"
#include "../include/scene/planet/SurfaceReflection.h"
#include "../include/scene/vegetation/Plant.h"
#include "../include/extra/MouseShootFeature.h"
#include "../include/scene/vegetation/Tree.h"
#include "../include/scene/vegetation/Grass.h"
#include "../include/scene/character/OBJCharacter.h"
#include "../include/scene/character/KeyboardCharacterControl.h"

static constexpr const char* WINDOW_TITLE = "Gamedev3d";

void loadFromFile(ISerializer* serializer, std::shared_ptr<IWindow> window) {
	serializer->addFactory(WASDCamera::factory());
	serializer->addFactory(ChaseCamera::factory());
	serializer->addFactory(Sky::factory());
	serializer->addFactory(ShadowMap::factory());
	serializer->addFactory(Renderer2d::factory());
	serializer->addFactory(SurfaceReflection::factory());
	serializer->addFactory(Texture::factory());
	serializer->addFactory(Planet::factory());
	serializer->addFactory(FourWheels::factory());
	serializer->addFactory(Road::factory());
	serializer->addFactory(Clouds::factory());
	serializer->addFactory(SphereShape::factory());
	serializer->addFactory(Plant::factory());
	serializer->addFactory(Tree::factory());
	serializer->addFactory(Grass::factory());

	serializer->open();

	std::string className;
	unsigned long objectId;
	while (serializer->readBegin(className, objectId)) {
		Log::debug("CREATING %s", className.c_str());
		const Factory& factory = serializer->getFactory(className);
		factory.create(objectId, serializer, window);
	}
	serializer->close();
}

//-----------------------------------------------------------------------------

void buildNewScene(IGameScene* app, std::shared_ptr<IWindow> window) {
	float planetRadius = 6000.f;
	float waterLevel = planetRadius - 0.2f;

	// Editor Camera
	{
		std::shared_ptr<Camera> camera = std::make_shared<WASDCamera>(window);
		camera->setFovY(50.f); //better = 60.f
		camera->setPosition(btVector3(planetRadius + 300.f, 0.f, 0.f));
		camera->setTargetPosition(btVector3(0.f, 0.f, 0.f));
		app->setFlyCamera(camera);
		app->addMouseListener(std::dynamic_pointer_cast<IMouseListener>(camera));
		app->addKeyboardListener(std::dynamic_pointer_cast<IKeyboardListener>(camera));
		app->addSerializable(camera);
		window->addResizeListener([camera](int w, int h) { camera->windowResized(); });
	}

	// SurfaceReflection
	{
		std::shared_ptr<ISurfaceReflection> surfaceReflection = std::make_shared<SurfaceReflection>(planetRadius, waterLevel, window->getWidth(), window->getHeight());
		app->setSurfaceReflection(surfaceReflection);
		app->addSerializable(surfaceReflection);
	}

	// ShadowMap
	{
		std::shared_ptr<ShadowMap> shadowMap = std::make_shared<ShadowMap>(0, 8192);
		app->setShadowMap(shadowMap);
		app->addSerializable(shadowMap);
	}

	// Renderer2d
	{
		std::shared_ptr<IRenderer2d> renderer2d = std::make_shared<Renderer2d>(window);
		app->setRenderer2d(renderer2d);
		app->addSerializable(renderer2d);
	}

	// planet
	Log::info("Loading Planet");
//	std::shared_ptr<Planet> planet = std::make_shared<Planet>(app->getDynamicsWorld(), planetRadius, waterLevel, 29, 32);
//	std::shared_ptr<Planet> planet = std::make_shared<Planet>(app->getDynamicsWorld(), planetRadius, waterLevel, 17, 32);
	std::shared_ptr<Planet> planet = std::make_shared<Planet>(app->getDynamicsWorld(), planetRadius, waterLevel, 9, 32);

	std::unique_ptr<ITexture> mat0 = std::make_unique<Texture>(2, IoUtils::resource("/texture/grass.jpg"));
	mat0->mipmap()->repeat()->scaleFactor(325.f);
	planet->setTexture(0, std::move(mat0));

	std::unique_ptr<ITexture> mat1 = std::make_unique<Texture>(3, IoUtils::resource("/texture/dirt.jpg"));
	mat1->mipmap()->repeat()->scaleFactor(200.f);
	planet->setTexture(1, std::move(mat1));

	std::unique_ptr<ITexture> mat2 = std::make_unique<Texture>(4, IoUtils::resource("/texture/stone.jpg"));
	mat2->mipmap()->repeat()->scaleFactor(100.f);
	planet->setTexture(2, std::move(mat2));

	std::unique_ptr<ITexture> mat3 = std::make_unique<Texture>(5, IoUtils::resource("/texture/sidewalk.jpg"));
	mat3->mipmap()->repeat()->scaleFactor(100.f);
	planet->setTexture(3, std::move(mat3));

	planet->initPhysics(btTransform::getIdentity());
	app->addSceneObject(planet);

	// Car
	Log::info("Loading car");
	const btVector3& carPosition = btVector3(planet->getHeightAt({1.0f, 0.0f, 0.0f}), 0.f, 0.f);
	std::shared_ptr<FourWheels> car = std::make_shared<FourWheels>(app->getDynamicsWorld(), carPosition, "/model/vehicle/pickup_lowpoly.obj", "/model/vehicle/truck_wheel.obj");
	car->initPhysics(btTransform::getIdentity());
	car->getSharedRigidBody()->setWorldTransform(planet->getSurfaceTransform({1.0f, 0.0f, 0.0f}));

	app->addSceneObject(car);
	// terrain-car contact
	planet->interactWith(car->getSharedRigidBody());

	// Car control
	{
		auto&& vehicleControl = std::make_shared<KeyboardVehicleControl>(car, SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_SPACE);
		app->addKeyboardListener(vehicleControl);
	}


	// Roads
	{
		auto roadMat0 = std::make_unique<Texture>(2, IoUtils::resource("/texture/asphalt.jpg"));
		roadMat0->mipmap()->repeat()->scaleFactor(1500.f);

		auto blockMat0 = std::make_unique<Texture>(2, IoUtils::resource("/texture/sidewalk.jpg"));
		blockMat0->mipmap()->repeat()->scaleFactor(100.f);

		std::shared_ptr<Road> road = std::make_shared<Road>(app->getDynamicsWorld(), planet, std::move(roadMat0), std::move(blockMat0));
		road->initPhysics(btTransform::getIdentity());
		app->addSceneObject(road);
	}

	// Plants
	{
		std::shared_ptr<Plant> plant = std::make_shared<Plant>(planet);
		app->addSerializable(plant);
		app->addCommands(plant->getCommands());
		planet->addExternalObject(plant);
	}

	// Trees
	{
		std::shared_ptr<Tree> tree = std::make_shared<Tree>(planet);

		tree->addModel("1", "/model/vegetation/treeA.obj", 400.0f, 1);
		tree->addModel("1", "/model/vegetation/treeA_LOD1.obj", 800.0f, 1);
		tree->addModel("1", "/model/vegetation/treeA_LOD2.obj", 99999.0f, 1);

		tree->addModel("2", "/model/vegetation/treeB.obj", 400.0f, 0);
		tree->addModel("2", "/model/vegetation/treeB_LOD1.obj", 800.0f, 0);
		tree->addModel("2", "/model/vegetation/treeB_LOD2.obj", 99999.0f, 0);

		tree->addModel("3", "/model/vegetation/treeC.obj", 400.0f, 1);
		tree->addModel("3", "/model/vegetation/treeC_LOD1.obj", 800.0f, 1);
		tree->addModel("3", "/model/vegetation/treeC_LOD2.obj", 99999.0f, 1);

		app->addSerializable(tree);
		app->addCommands(tree->getCommands());
		planet->addExternalObject(tree);
	}

	// Grasses
	{
		std::shared_ptr<Grass> grass = std::make_shared<Grass>(planet, "/model/vegetation/grass.obj");
		app->addCommands(grass->getCommands());
		app->addSerializable(grass);
		planet->addExternalObject(grass);
	}

	// Sky
	{
		std::shared_ptr<Sky> sky = std::make_shared<Sky>(planetRadius, waterLevel, btVector3(30000.f, 0.f, 0.f));
		app->setSky(sky);
		app->addSceneObject(sky);
	}

	// Clouds
	{
		std::shared_ptr<Clouds> clouds = std::make_shared<Clouds>(std::make_pair(2000.f, 2500.f));
		clouds->createClouds(planetRadius, 700.f, 500.f, 100);
		clouds->createClouds(planetRadius, 800.f, 600.f, 100);
		clouds->bind();
		app->addSceneObject(clouds);
	}

	// OBJCharacter

	std::unique_ptr<OBJAnimation> stand = std::make_unique<OBJAnimation>(1.0f);
	stand->addFrames("/model/dummy/stand", ".obj", 1, 1);

	std::unique_ptr<OBJAnimation> walk = std::make_unique<OBJAnimation>(30.0f);
	walk->addFrames("/model/dummy/walk/walk_0000", ".obj", 1, 27);

	const btTransform&& transform = planet->getSurfaceTransform(btVector3(1.001f, 0.0f, 0.0f), 0.0f);

	std::shared_ptr<OBJCharacter> character = std::make_shared<OBJCharacter>(transform, planet);
	// Camera adjustments
	character->setCameraHeight(4.0f);
	character->setCameraDistanceRange(8.0f, 12.0f);
	character->setVehicleCameraHeight(15.0f);
	character->setVehicleCameraDistanceRange(20.0f, 30.0f);
	// Animations
	character->addAnimation(std::move(stand));
	character->addAnimation(std::move(walk));
	app->addSceneObject(character);

	// Character Control
	{
		auto&& characterControl = std::make_shared<KeyboardCharacterControl>(character, SDLK_w, SDLK_s, SDLK_a, SDLK_d);
		characterControl->addAction(0, SDLK_UNKNOWN, 0.0f); // walk
		characterControl->addAction(1, SDLK_UNKNOWN, 0.0625f); // walk

		characterControl->enableVehicles(SDLK_e);
		characterControl->addVehicle(car);

		app->addKeyboardListener(characterControl);
	}

	// Game Camera / Chase Camera
	{
		std::shared_ptr<Camera> camera = std::make_shared<ChaseCamera>(window, character, planet);
		camera->setFovY(50.f);
		camera->setPosition(carPosition + btVector3(2.0f, 2.0f, 2.0f));
		app->setGameCamera(camera);
		app->addSerializable(camera);
		window->addResizeListener([camera](int w, int h) { camera->windowResized(); });
	}
}

//-----------------------------------------------------------------------------

void addExtraFeatures(std::shared_ptr<IGameScene> app, std::shared_ptr<IWindow> window) {
	std::shared_ptr<Planet> planet = std::dynamic_pointer_cast<Planet>(window->getGameScene()->getSerializable(Planet::SERIALIZE_ID));
	auto mouseShootFeature = std::make_shared<MouseShootFeature>(app, planet, IMouseListener::ButtonCode::RIGHT_BUTTON, "/model/soccer/soccerball.obj");
	app->addMouseListener(std::dynamic_pointer_cast<IMouseListener>(mouseShootFeature));
}

//-----------------------------------------------------------------------------

#define IS_FULLSCREEN false
#define LOAD_FROM_FILE false


int main(int argc, char** argv)
{
	std::shared_ptr<IWindow> window;
	try {
		window = std::make_shared<Window>(WINDOW_TITLE, 1200, 800, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
		if (IS_FULLSCREEN) {
			window->setFullScreen();
		}
	} catch (const std::runtime_error& e) {
		Log::error(e.what());
		return -1;
	}
	SDL_ShowCursor(SDL_DISABLE);

	std::shared_ptr<IGameScene> app = std::make_shared<Application>();
	window->setGameScene(app);

	std::shared_ptr<ISerializer> serializer = std::make_shared<Serializer>("/Users/hugo/planet0.bin");
	app->enableSerialization(serializer);

	if (LOAD_FROM_FILE) {
		loadFromFile(serializer.get(), window);
	} else {
		buildNewScene(app.get(), window);
	}
	addExtraFeatures(app, window);

	window->loop();

	Log::debug("The End");

	return 0;
}



