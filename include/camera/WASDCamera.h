#ifndef WASDCAMERA_H_
#define WASDCAMERA_H_

#include <SDL2/SDL_keycode.h>
#include "../app/Interfaces.h"
#include "Camera.h"


class WASDCamera: public Camera, public IMouseListener, public IKeyboardListener {

	bool w, a, s, d, e, q, shiftDown, ctrlDown;

public:
	static const std::string& SERIALIZE_ID;

	WASDCamera(std::weak_ptr<IWindow> window):
		Camera(std::move(window)),
		IMouseListener(),
		IKeyboardListener(),
		w(false),
		a(false),
		s(false),
		d(false),
		e(false),
		q(false),
		shiftDown(false),
		ctrlDown(false)
	{}

	virtual ~WASDCamera()
	{ Log::debug("Deleting WASDCamera"); }

	void updateControls() override;
	void keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	void keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) override;
	void mouseMove(Sint32 x, Sint32 y, const GameState& gameState) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

#endif
