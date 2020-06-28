#include "Application.h"


Application::Application() {
	mCollisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
	mDispatcher = std::make_unique<btCollisionDispatcher>(mCollisionConfiguration.get());
	const btVector3 worldMin(-1000,-1000,-1000);
	const btVector3 worldMax(1000,1000,1000);
	mOverlappingPairCache = std::make_unique<btAxisSweep3>(worldMin,worldMax);
	mConstraintSolver = std::make_unique<btSequentialImpulseConstraintSolver>();

	mDynamicsWorld = std::make_shared<btDiscreteDynamicsWorld>(mDispatcher.get(), mOverlappingPairCache.get(), mConstraintSolver.get(), mCollisionConfiguration.get());
}


Application::~Application() {
	Log::debug("Deleting Application");
	// Clear vectors / force garbage collection before physics world is deleted
	mSerializables.clear();
	mSceneObjects.clear();
	mMouseListeners.clear();
	mKeyboardListeners.clear();

	Log::debug("Deleting physics world");
	// Clean up collision objects, if any
	for (int i = mDynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
		Log::debug("removeCollisionObject %d", i);
		mDynamicsWorld->removeCollisionObject(mDynamicsWorld->getCollisionObjectArray()[i]);
	}
}


std::shared_ptr<ISerializable> Application::getSerializable(const std::string& serializeID) const {
	for (auto& sceneObject : mSceneObjects) {
		if (serializeID == sceneObject->serializeID())
			return sceneObject;
	}
	return nullptr;
}


std::shared_ptr<ISceneObject> Application::getByObjectId(unsigned long objectId) const {
	for (auto& sceneObject : mSceneObjects) {
		if (objectId == sceneObject->getObjectId())
			return sceneObject;
	}
	return nullptr;
}


void Application::addSceneObject(std::shared_ptr<ISceneObject> sceneObject) {
	addSerializable(sceneObject);

	mSceneObjects.push_front(sceneObject);
	addCommands(sceneObject->getCommands());
}


void Application::addCommands(ISceneObject::CommandMap commands) {
	for (const auto& entry : commands) {
		if (mCommandMap.find(entry.first) != mCommandMap.end())
			throw std::runtime_error("Duplicate command: " + entry.first);

		mCommandMap[entry.first] = entry.second;
	}
}


void Application::renderScenePass(const ISurfaceReflection* surfaceReflection)
{
	for (auto& sceneObject : mSceneObjects) {
		sceneObject->renderOpaque(getActiveCamera(), mSky.get(), mShadowMap.get(), surfaceReflection, mGameState);
	}
}


void Application::renderTranslucentPass(const ISurfaceReflection* surfaceReflection)
{
	// Make depth buffer read only
	glDepthMask(GL_FALSE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	for (auto& sceneObject : mSceneObjects) {
		sceneObject->renderTranslucent(getActiveCamera(), mSky.get(), mShadowMap.get(), surfaceReflection, mGameState);
	}
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);

	// Make depth buffer read write
	glDepthMask(GL_TRUE);
}


void Application::renderScene(ICamera* pCamera)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_CULL_FACE);

	bool isShadowEnabled = mGameState.debugCode != DebugCode::NO_SHADOW;
	if (isShadowEnabled && mShadowMap)
	{
		const btVector3& vCameraDir1 = pCamera->getDirectionUnit();
		const btVector3& vOriginToSun1 = - mSky->getSunPosition().normalized();
		const btVector3& target = pCamera->getPosition() + vCameraDir1 * 200.f;
		const btVector3& lightPosition = target - vOriginToSun1 * 500.f;

		mShadowMap->start(lightPosition, target, vCameraDir1);
		renderScenePass(nullptr);
//		renderTranslucentPass(nullptr);
		mShadowMap->end();

		// restore viewport because the shadowMap has changed it (see Framebuffer.startDepth())
		pCamera->setViewport();
	}

	if (mSurfaceReflection) {
		mSurfaceReflection->begin();
		pCamera->update();
		renderScenePass(mSurfaceReflection.get());
		renderTranslucentPass(mSurfaceReflection.get());
		mSurfaceReflection->end();

		// restore viewport because the shadowMap has changed it (see Framebuffer.startColorAndDepth())
		pCamera->setViewport();
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	pCamera->update();

	static int lastX, lastY;

	renderScenePass(mSurfaceReflection.get());
	renderTranslucentPass(mSurfaceReflection.get());

	// Capture the mouse position 3D after rendering the scene
	// (we can't get the 3d point before rendering for obvious reasons)
	bool pointHasChanged = lastX != mGameState.mouseX || lastY != mGameState.mouseY;
	if (mGameState.mode == GameMode::EDITING && pointHasChanged) {
		mGameState.mouse3d = pCamera->getPointAt(mGameState.mouseX, mGameState.mouseY);
		lastX = mGameState.mouseX;
		lastY = mGameState.mouseY;
	}

	if (mRenderer2d) {
		mRenderer2d->begin();

		const static SDL_Color BLACK = { 0, 0, 0 };
		const static SDL_Color WHITE = { 1, 1, 1 };
		for (auto& object : mSceneObjects)
			object->render2d(mRenderer2d.get());

//		if (mGameState.isDebugging) {
			static FPSCounter fps;
			mRenderer2d->renderText(10, std::to_string(fps.getFps()) + " FPS", BLACK, "left=5", "bottom=0", 14);
//		}

		if (mGameState.mode == GameMode::EDITING) {
			mRenderer2d->renderCursorAt(mGameState.mouseX, mGameState.mouseY);
			mRenderer2d->renderText(20, "Edit Mode", BLACK, "center", "top=40", 16);
			mRenderer2d->renderText(21, ">" + mCommand + "_", BLACK, "left=5", "top=50", 16);
		} else if (mGameState.mode == GameMode::PAUSED) {
			mRenderer2d->renderText(30, "Pause", BLACK, "center", "top=10", 30);
		}

		if (mGameState.isFreeFly)
			mRenderer2d->renderText(40, "FREE FLY MODE", BLACK, "right=150", "top=5", 16);

		if (mGameState.debugCode == DebugCode::NO_SHADOW)
			mRenderer2d->renderText(50, "NO_SHADOW", BLACK, "right=150", "top=20", 16);
		else if (mGameState.debugCode == DebugCode::COLLISION_SHAPE)
			mRenderer2d->renderText(60, "COLLISION_SHAPE", BLACK, "right=150", "top=20", 16);
		else if (mSurfaceReflection && mGameState.debugCode == DebugCode::WATER_REFLECTION) {
			mRenderer2d->renderImageFlipY(mSurfaceReflection->getColorTexture(), 0.f, 0.f);
			mRenderer2d->renderText(70, "WATER_REFLECTION", WHITE, "center", "top=20", 16);
			mRenderer2d->renderText(71, "WATER_REFLECTION", WHITE, "center", "bottom=20", 16);
		} else if (mGameState.debugCode == DebugCode::SIMPLIFIED)
			mRenderer2d->renderText(80, "SIMPLIFIED", BLACK, "right=150", "top=20", 16);

		mRenderer2d->end();
	}
}


void Application::moveAndDisplay() {
	ICamera* pCamera = getActiveCamera();

	for (auto& listener : mKeyboardListeners)
		listener->update(pCamera);

	if (mGameState.mode == GameMode::RUNNING) {
		static constexpr const int maxSimSubSteps = 2;
		double dt = getDeltaTimeMicroseconds() * 0.000001;
		mDynamicsWorld->stepSimulation(dt, maxSimSubSteps);
	}

	for (auto& sceneObject : mSceneObjects)
		sceneObject->updateSimulation();

	renderScene(pCamera);
}


void Application::handleCommand(SDL_Keycode key) {
	static std::string lastCommand;
	if (key == SDLK_BACKSPACE) {
		std::string::size_type len = mCommand.length();
		if (len > 0)
			mCommand.erase(len - 1);
	} else if (key == SDLK_UP) {
		mCommand = lastCommand;
	} else if (key == SDLK_RETURN) {
		std::string commandName = mCommand;
		std::string parameters;
		std::string::size_type posSpace = commandName.find(' ');
		if (posSpace != std::string::npos) {
			parameters = mCommand.substr(posSpace+1);
			commandName.erase(posSpace);
		}
		auto command = std::find_if(mCommandMap.begin(), mCommandMap.end(), [&commandName] (const auto& e) { return e.first == commandName; });
		if (command != mCommandMap.end()) {
			command->second(parameters);
		} else
			Log::error("Command not found: %s", mCommand.c_str());
		lastCommand = mCommand;
		mCommand.clear();
	} else {
		char c = (char) key;
		bool isNumber = c >= '0' && c <= '9';
		bool isLower = c >= 'a' && c <= 'z';
		bool isUpper = c >= 'A' && c <= 'Z';
		if (isNumber || isLower || isUpper || key == SDLK_SPACE || key == SDLK_PERIOD || key == SDLK_MINUS)
			mCommand += key;
	}
}


void Application::toggleFreeFly() {
	if (mGameState.isFreeFly)
		mGameCamera->copyFrom(mFlyCamera.get());
	else
		mFlyCamera->copyFrom(mGameCamera.get());

	mGameState.isFreeFly = !mGameState.isFreeFly;
}


void Application::keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt)
{
	if (key == SDLK_BACKSLASH) {
		mCommand.clear();
		mGameState.mode = mGameState.mode == GameMode::RUNNING? GameMode::EDITING : GameMode::RUNNING;
		bool isPaused = mGameState.mode != GameMode::RUNNING;
		mGameCamera->setPause(isPaused);
		mFlyCamera->setPause(isPaused);
	} else if (mGameState.mode == GameMode::EDITING) {
		handleCommand(key);
	} else {
		for (auto& listener : mKeyboardListeners) {
			listener->keyDown(key, shift, ctrl, alt);
		}
		if (ctrl) {
			switch(key) {
				case SDLK_r: mSky->setAmbientLight(mSky->getAmbientLight() + .1f); mSky->debug(); break;
				case SDLK_f: mSky->setAmbientLight(mSky->getAmbientLight() - .1f); mSky->debug(); break;
				case SDLK_t: mSky->setDiffuseLight(mSky->getDiffuseLight() + .1f); mSky->debug(); break;
				case SDLK_g: mSky->setDiffuseLight(mSky->getDiffuseLight() - .1f); mSky->debug(); break;
				case SDLK_y: mSky->setSpecularLight(mSky->getSpecularLight() + .1f); mSky->debug(); break;
				case SDLK_h: mSky->setSpecularLight(mSky->getSpecularLight() - .1f); mSky->debug(); break;
				case SDLK_u: mSky->setSunPosition(mSky->getSunPosition().rotate(btVector3(0.f, 1.f, 0.f), .1f)); mSky->debug(); break;
				default: break;
			}
		} else {
			switch(key) {
				case SDLK_1: mGameState.debugCode = DebugCode::NONE; break;
				case SDLK_2: mGameState.debugCode = DebugCode::COLLISION_SHAPE; break;
				case SDLK_3: mGameState.debugCode = DebugCode::NO_SHADOW; break;
				case SDLK_4: mGameState.debugCode = DebugCode::WATER_REFLECTION; break;
				case SDLK_5: mGameState.debugCode = DebugCode::SIMPLIFIED; break;
				case SDLK_r: resetScene(); break;
				case SDLK_0: toggleFreeFly(); break;
				default: break;
			}
		}
	}
}


void Application::keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt)
{
	for (auto& listener : mKeyboardListeners) {
		listener->keyUp(key, shift, ctrl, alt);
	}
}


void Application::mouseDown(Uint8 button, int x, int y)
{
	mGameState.isLeftMouseDown = button == static_cast<int>(IMouseListener::ButtonCode::LEFT_BUTTON);
	mGameState.isRightMouseDown = button == static_cast<int>(IMouseListener::ButtonCode::RIGHT_BUTTON);
	for (auto& listener : mMouseListeners) {
		listener->mouseDown(button, x, y, mGameState);
	}
}


void Application::mouseUp(Uint8 button, int x, int y)
{
	mGameState.isLeftMouseDown = button == static_cast<int>(IMouseListener::ButtonCode::LEFT_BUTTON) ? false : mGameState.isLeftMouseDown;
	mGameState.isRightMouseDown = button == static_cast<int>(IMouseListener::ButtonCode::RIGHT_BUTTON) ? false : mGameState.isRightMouseDown;
	for (auto& listener : mMouseListeners) {
		listener->mouseUp(button, x, y, mGameState);
	}
}


void Application::mouseMove(int x, int y)
{
	mGameState.mouseX = x;
	mGameState.mouseY = y;
	for (auto& listener : mMouseListeners) {
		listener->mouseMove(x, y, mGameState);
	}
}


void Application::resetScene()
{
	for (auto& sceneObject : mSceneObjects)
		sceneObject->reset();

	for (auto& listener : mMouseListeners)
		listener->reset();

	for (auto& listener : mKeyboardListeners)
		listener->reset();

	int numConstraints = mDynamicsWorld->getNumConstraints();
	for (int i = 0; i < numConstraints; i++) {
		mDynamicsWorld->getConstraint(i)->setEnabled(true);
	}

	// create a copy of the array, not a reference!
	btCollisionObjectArray copyArray = mDynamicsWorld->getCollisionObjectArray();

	for (int i = 0; i < copyArray.size(); i++) {
		btCollisionObject* colObj = copyArray[i];
		btRigidBody* body = btRigidBody::upcast(colObj);
		if (body) {
			if (body->getMotionState()) {
				btDefaultMotionState* myMotionState = (btDefaultMotionState*)body->getMotionState();
				myMotionState->m_graphicsWorldTrans = myMotionState->m_startWorldTrans;
				body->setCenterOfMassTransform( myMotionState->m_graphicsWorldTrans );
				colObj->setInterpolationWorldTransform(myMotionState->m_startWorldTrans);
				colObj->forceActivationState(ACTIVE_TAG);
				colObj->activate();
				colObj->setDeactivationTime(0);
				//colObj->setActivationState(WANTS_DEACTIVATION);
			}
			//removed cached contact points (this is not necessary if all objects have been removed from the dynamics world)
			if (auto cache = mDynamicsWorld->getBroadphase()->getOverlappingPairCache())
				cache->cleanProxyFromPairs(colObj->getBroadphaseHandle(), getDynamicsWorld()->getDispatcher());

			if (!body->isStaticObject()) {
				body->setLinearVelocity(btVector3(0,0,0));
				body->setAngularVelocity(btVector3(0,0,0));
			}
		}
	}

	// reset some internal cached data in the broadphase
	mDynamicsWorld->getBroadphase()->resetPool(getDynamicsWorld()->getDispatcher());
	mDynamicsWorld->getConstraintSolver()->reset();
}



