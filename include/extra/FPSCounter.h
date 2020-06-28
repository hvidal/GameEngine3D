#ifndef FPSCOUNTER_H
#define FPSCOUNTER_H

#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_timer.h>

class FPSCounter {

	// Frames per second
	Uint32 mLastTime{0};
	Uint32 mFpsCount{0};
	Uint32 mCurrentFps{0};

	void updateFps();
public:
	int getFps();
};

#endif