#ifndef WINDOW_H
#define WINDOW_H

#include <string>
#include <forward_list>
#include <SDL2/SDL.h>
#include "Interfaces.h"


class Window: public IWindow {
private:
	unsigned int mWidth;
	unsigned int mHeight;
	SDL_Window* mWindow;
	SDL_GLContext mContext;

	std::shared_ptr<IGameScene> mGameScene;

	std::forward_list<std::function<void(int w, int h)>> mWindowResizeListeners;

	void swap() const;
public:
	Window(const char* title, unsigned int width, unsigned int height, Uint32 flags);
	virtual ~Window();

	virtual unsigned int getWidth() const noexcept override;
	virtual unsigned int getHeight() const noexcept override;
	virtual float getAspectRatio() const noexcept override;
	virtual void resize(unsigned int width, unsigned int height) override;
	virtual void centerMouseCursor() const override;
	virtual void setFullScreen() override;
	virtual void setGameScene(std::shared_ptr<IGameScene>) override;
	virtual std::shared_ptr<IGameScene> getGameScene() const override;
	virtual void addResizeListener(std::function<void(int, int)>) override;

	virtual void keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt);
	virtual void keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt);
	virtual void mouseDown(Uint8 button, int x, int y);
	virtual void mouseUp(Uint8 button, int x, int y);
	virtual void mouseMove(int x, int y);

	virtual void loop() override;
};

//-----------------------------------------------------------------------------

inline unsigned int Window::getWidth() const noexcept
{ return mWidth; }

inline unsigned int Window::getHeight() const noexcept
{ return mHeight; }

inline float Window::getAspectRatio() const noexcept
{ return mWidth / (float) mHeight; }

inline void Window::setGameScene(std::shared_ptr<IGameScene> gameScene)
{ mGameScene = gameScene; }

inline std::shared_ptr<IGameScene> Window::getGameScene() const
{ return mGameScene; }

inline void Window::centerMouseCursor() const
{ SDL_WarpMouseInWindow(mWindow, mWidth/2, mHeight/2); }

inline void Window::keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt)
{ mGameScene->keyDown(key, shift, ctrl, alt); }

inline void Window::keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt)
{ mGameScene->keyUp(key, shift, ctrl, alt); }

inline void Window::mouseDown(Uint8 button, int x, int y)
{ mGameScene->mouseDown(button, x, y); }

inline void Window::mouseUp(Uint8 button, int x, int y)
{ mGameScene->mouseUp(button, x, y); }

inline void Window::mouseMove(int x, int y)
{ mGameScene->mouseMove(x, y); }

inline void Window::addResizeListener(std::function<void(int, int)> listener) {
	mWindowResizeListeners.push_front(listener);
}

#endif