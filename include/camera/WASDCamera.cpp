#include "WASDCamera.h"

const std::string& WASDCamera::SERIALIZE_ID = "WASDCamera";


void WASDCamera::updateControls() {
	float factor = shiftDown? 10.f : ctrlDown? 0.1f : 1.f;
	bool isModified = false;
	if (w) {
		btVector3&& inc = getDirectionUnit() * factor;
		mPosition += inc;
		mTargetPosition += inc;
		isModified = true;
	}
	if (s) {
		btVector3&& inc = getDirectionUnit() * factor;
		mPosition -= inc;
		mTargetPosition -= inc;
		isModified = true;
	}
	if (a) {
		btVector3&& toRight = getDirectionUnit().cross(mUp);
		btVector3&& incRight = toRight * factor;
		mPosition -= incRight;
		mTargetPosition -= incRight;
		isModified = true;
	}
	if (d) {
		btVector3 toRight = getDirectionUnit().cross(mUp);
		btVector3&& incRight = toRight * factor;
		mPosition += incRight;
		mTargetPosition += incRight;
		isModified = true;
	}
	if (e) {
		mUp = mUp.rotate(getDirectionUnit(), .02f).normalized();
		isModified = true;
	}
	if (q) {
		mUp = mUp.rotate(getDirectionUnit(), -.02f).normalized();
		isModified = true;
	}

	if (isModified)
		mVersion++;
	//Log::debug("Camera POS %.1f %.1f %.1f | TARGET %.1f %.1f %.1f", mPosition.x(), mPosition.y(), mPosition.z(), mTargetPosition.x(), mTargetPosition.y(), mTargetPosition.z());
}


void WASDCamera::keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) {
	switch(key) {
		case SDLK_w:
			w = true;
			break;
		case SDLK_s:
			s = true;
			break;
		case SDLK_a:
			a = true;
			break;
		case SDLK_d:
			d = true;
			break;
		case SDLK_e:
			e = true;
			break;
		case SDLK_q:
			q = true;
			break;
	}
	shiftDown = shift;
	ctrlDown = ctrl;
}


void WASDCamera::keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) {
	switch(key) {
		case SDLK_w:
			w = false;
			break;
		case SDLK_s:
			s = false;
			break;
		case SDLK_a:
			a = false;
			break;
		case SDLK_d:
			d = false;
			break;
		case SDLK_e:
			e = false;
			break;
		case SDLK_q:
			q = false;
			break;
	}
	shiftDown = shift;
	ctrlDown = ctrl;
}


void WASDCamera::mouseMove(Sint32 x, Sint32 y, const GameState& gameState) {
	if (mPaused)
		return;

	auto window = mWindow.lock();

	window->centerMouseCursor();
	float centerX = window->getWidth() / 2.f;
	float centerY = window->getHeight() / 2.f;
	float diffX = (centerX - x) / 250.f;
	float diffY = (centerY - y) / 250.f;

	btVector3&& direction = getDirection().rotate(mUp, diffX);
	btVector3&& toTheSide = direction.cross(mUp).normalized();
	direction = direction.rotate(toTheSide, diffY);
	mUp = mUp.rotate(toTheSide, diffY).normalized();
	mTargetPosition = mPosition + direction;
	mVersion++;
}


void WASDCamera::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	writeVars(serializer);
}


std::pair<std::string, Factory> WASDCamera::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		std::shared_ptr<WASDCamera> o = std::make_shared<WASDCamera>(window);
		o->setObjectId(objectId);
		o->readVars(serializer);

		IGameScene* pGameScene = window->getGameScene().get();
		pGameScene->setFlyCamera(o);
		pGameScene->addMouseListener(o);
		pGameScene->addKeyboardListener(o);
		pGameScene->addSerializable(o);
		window->addResizeListener([o](int w, int h) { o->windowResized(); });
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& WASDCamera::serializeID() const noexcept {
	return SERIALIZE_ID;
}
