#include <OpenGL/gl3.h>
#include <future>
#include <SDL2/SDL_video.h>
#include "Window.h"


Window::Window(const char* title, unsigned int width, unsigned int height, Uint32 flags):
	mWindow(nullptr),
	mContext(nullptr),
	mWidth(width),
	mHeight(height)
{
	//initialize all SDL subsystems
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
		throw std::runtime_error("SDL Init Failed");

	/* and make sure we specify a GL version */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 16);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	mWindow = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWidth, mHeight, flags);
	if (mWindow == nullptr)
		throw std::runtime_error("Failed to create window");

	// Create the context
	mContext = SDL_GL_CreateContext(mWindow);
	if (mContext == nullptr)
		throw std::runtime_error("Failed to create GL context");

	/* vsync | Make SwapInterval = ZERO because we need immediate updates (high FPS) */
	if (SDL_GL_SetSwapInterval(0) < 0)
		Log::info("WARN Unable to set swap interval");

	/* Next make the window current */
	SDL_GL_MakeCurrent(mWindow, mContext);

	Log::info("SDL init() finished");
}


Window::~Window() {
	SDL_GL_DeleteContext(mContext);
	SDL_DestroyWindow(mWindow);
	SDL_Quit();
}


void Window::setFullScreen() {
	SDL_DisplayMode current;
	int should_be_zero = SDL_GetCurrentDisplayMode(0, &current);
	if (should_be_zero != 0) {
		Log::debug("Could not get display mode for video display: %s", SDL_GetError());
		throw std::runtime_error("Fullscreen mode failed");
	}
	mWidth = static_cast<unsigned int>(current.w);
	mHeight = static_cast<unsigned int>(current.h);
	SDL_SetWindowFullscreen(mWindow, SDL_WINDOW_FULLSCREEN);
	SDL_SetWindowSize(mWindow, mWidth, mHeight);
	Log::debug("Fullscreen mode: %d x %d", mWidth, mHeight);
}


void Window::resize(unsigned int width, unsigned int height) {
	mWidth = width;
	mHeight = height;
	glViewport(0, 0, mWidth, mHeight);
	for (auto listener : mWindowResizeListeners) {
		listener(mWidth, mHeight);
	}
}


void Window::swap() const {
	SDL_Delay(10);
	SDL_GL_SwapWindow(mWindow);
}


void Window::loop() {
	Log::debug("Starting main loop");
	bool quit = false;

	auto handleEvent = [this, &quit](SDL_Event e) {
		bool shift, ctrl, alt;
		Uint16 mod = e.key.keysym.mod;

		switch(e.type) {
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_WINDOWEVENT:
				if (e.window.event == SDL_WINDOWEVENT_RESIZED) {
					resize(e.window.data1, e.window.data2);
					Log::debug("Resized to %d x %d", getWidth(), getHeight());
				}
				break;
			case SDL_MOUSEMOTION:
				mouseMove(e.motion.x, e.motion.y);
				break;
			case SDL_MOUSEBUTTONDOWN:
				mouseDown(e.button.button, e.button.x, e.button.y);
				break;
			case SDL_MOUSEBUTTONUP:
				mouseUp(e.button.button, e.button.x, e.button.y);
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE) {
					quit = true;
					break;
				} else if (e.key.keysym.sym == SDLK_SLASH) {
					setFullScreen();
					break;
				}
				shift = (mod & KMOD_RSHIFT) || (mod & KMOD_LSHIFT);
				ctrl = (mod & KMOD_RCTRL) || (mod & KMOD_LCTRL);
				alt = (mod & KMOD_RALT) || (mod & KMOD_LALT);
				keyDown(e.key.keysym.sym, shift, ctrl, alt);
				break;
			case SDL_KEYUP:
				shift = (mod & KMOD_RSHIFT) || (mod & KMOD_LSHIFT);
				ctrl = (mod & KMOD_RCTRL) || (mod & KMOD_LCTRL);
				alt = (mod & KMOD_RALT) || (mod & KMOD_LALT);
				keyUp(e.key.keysym.sym, shift, ctrl, alt);
				break;
		}
	};

	SDL_Event e;
	unsigned int c = 0;

	// start main loop
	while (!quit) {
		// optimization: we don't have to poll events every frame
		if (c++ % 2 == 0) {
			while (SDL_PollEvent(&e))
				handleEvent(e);
		}
		mGameScene->moveAndDisplay();
		swap();
	}
}

