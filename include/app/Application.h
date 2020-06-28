#ifndef APPLICATION_H
#define APPLICATION_H

#include <vector>
#include <list>
#include <forward_list>
#include "BulletDynamics/Dynamics/btDynamicsWorld.h"
#include "BulletCollision/CollisionShapes/btCollisionShape.h"
#include "BulletCollision/CollisionShapes/btBoxShape.h"
#include "BulletCollision/CollisionShapes/btSphereShape.h"
#include "BulletCollision/CollisionShapes/btCompoundShape.h"
#include "BulletCollision/CollisionShapes/btUniformScalingShape.h"
#include "BulletDynamics/ConstraintSolver/btConstraintSolver.h"
#include "LinearMath/btIDebugDraw.h"
#include "LinearMath/btQuickprof.h"
#include "LinearMath/btDefaultMotionState.h"

#include "Interfaces.h"
#include "../scene/SceneObject.h"
#include "../extra/FPSCounter.h"
#include "../util/Log.h"


class Application: public IGameScene
{
private:
	std::shared_ptr<ICamera> mGameCamera;
	std::shared_ptr<ICamera> mFlyCamera;
	std::shared_ptr<ISky> mSky;
	std::shared_ptr<IShadowMap> mShadowMap;
	std::shared_ptr<IRenderer2d> mRenderer2d;
	std::shared_ptr<ISurfaceReflection> mSurfaceReflection;

	std::forward_list<std::shared_ptr<ISceneObject>> mSceneObjects;
	std::forward_list<std::shared_ptr<IMouseListener>> mMouseListeners;
	std::forward_list<std::shared_ptr<IKeyboardListener>> mKeyboardListeners;
	std::list<std::shared_ptr<ISerializable>> mSerializables;

	std::shared_ptr<btDynamicsWorld> mDynamicsWorld;
	std::unique_ptr<btConstraintSolver> mConstraintSolver;
	std::unique_ptr<btBroadphaseInterface> mOverlappingPairCache;
	std::unique_ptr<btCollisionDispatcher> mDispatcher;
	std::unique_ptr<btDefaultCollisionConfiguration> mCollisionConfiguration;

	GameState mGameState;

	btClock mClock;
	unsigned long getDeltaTimeMicroseconds();

	std::string mCommand;
	ISceneObject::CommandMap mCommandMap;
	void handleCommand(SDL_Keycode key);

	void renderScene(ICamera* pCamera);
	void renderScenePass(const ISurfaceReflection* surfaceReflection);
	void renderTranslucentPass(const ISurfaceReflection* surfaceReflection);
	void resetScene();
	void toggleFreeFly();

public:
	Application();
	virtual ~Application();

	/* IGameScene */
	virtual std::shared_ptr<btDynamicsWorld> getDynamicsWorld() const noexcept override;
	virtual void setSky(std::shared_ptr<ISky>) noexcept override;
	virtual void setRenderer2d(std::shared_ptr<IRenderer2d>) noexcept override;
	virtual void setShadowMap(std::shared_ptr<IShadowMap>) noexcept override;
	virtual void setSurfaceReflection(std::shared_ptr<ISurfaceReflection>) noexcept override;
	virtual void setGameCamera(std::shared_ptr<ICamera>) noexcept override;
	virtual void setFlyCamera(std::shared_ptr<ICamera>) noexcept override;
	virtual void enableSerialization(std::shared_ptr<ISerializer>) override;
	virtual void addSerializable(std::shared_ptr<ISerializable>) noexcept override;
	virtual std::shared_ptr<ISerializable> getSerializable(const std::string&) const override;
	virtual std::shared_ptr<ISceneObject> getByObjectId(unsigned long) const override;
	virtual void addSceneObject(std::shared_ptr<ISceneObject>) override;
	virtual void addMouseListener(std::shared_ptr<IMouseListener>) noexcept override;
	virtual void addKeyboardListener(std::shared_ptr<IKeyboardListener>) noexcept override;
	virtual void addCommands(ISceneObject::CommandMap commands) override;
	virtual ICamera* getActiveCamera() const noexcept override;

	virtual void keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	virtual void keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	virtual void mouseDown(Uint8 button, int x, int y) override;
	virtual void mouseUp(Uint8 button, int x, int y) override;
	virtual void mouseMove(int x, int y) override;
	virtual void moveAndDisplay() override;
};

//-----------------------------------------------------------------------------

inline unsigned long Application::getDeltaTimeMicroseconds() {
	unsigned long dt = mClock.getTimeMicroseconds();
	mClock.reset();
	return dt;
}

inline std::shared_ptr<btDynamicsWorld> Application::getDynamicsWorld() const noexcept
{ return mDynamicsWorld; }

inline void Application::setSky(std::shared_ptr<ISky> sky) noexcept
{ mSky = sky; }

inline void Application::setShadowMap(std::shared_ptr<IShadowMap> shadowMap) noexcept
{ mShadowMap = shadowMap; }

inline void Application::setSurfaceReflection(std::shared_ptr<ISurfaceReflection> surfaceReflection) noexcept
{ mSurfaceReflection = surfaceReflection; }

inline void Application::setGameCamera(std::shared_ptr<ICamera> camera) noexcept
{ mGameCamera = camera; }

inline void Application::setFlyCamera(std::shared_ptr<ICamera> camera) noexcept
{ mFlyCamera = camera; }

inline void Application::setRenderer2d(std::shared_ptr<IRenderer2d> renderer2d) noexcept
{ mRenderer2d = renderer2d; }

inline void Application::addMouseListener(std::shared_ptr<IMouseListener>  listener) noexcept
{ mMouseListeners.push_front(listener); }

inline void Application::addKeyboardListener(std::shared_ptr<IKeyboardListener> listener) noexcept
{ mKeyboardListeners.push_front(listener); }

inline void Application::addSerializable(std::shared_ptr<ISerializable> serializable) noexcept
{ mSerializables.push_back(serializable); }

inline ICamera* Application::getActiveCamera() const noexcept
{ return mGameState.isFreeFly? mFlyCamera.get() : mGameCamera.get(); }

inline void Application::enableSerialization(std::shared_ptr<ISerializer> serializer)
{
	mCommandMap["save"] = [serializer, this](const std::string& p){
		Log::info("Saving...");
		ISerializer* pSerializer = serializer.get();
		pSerializer->open(true);
		for (auto& o : mSerializables) {
			o->write(pSerializer);
		}
		pSerializer->write(std::string("END"));
		pSerializer->close();
		Log::info("Saved");
	};
}

#endif

