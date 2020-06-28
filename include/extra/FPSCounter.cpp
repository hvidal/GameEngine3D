#include "FPSCounter.h"

void FPSCounter::updateFps() {
	Uint32 ticks = SDL_GetTicks();
	Uint32 diff = ticks - mLastTime;
	if (diff > 600) {
		mCurrentFps = (1000 * mFpsCount) / diff;
		mFpsCount = 0;
		mLastTime = ticks;
	} else
		mFpsCount++;
}

int FPSCounter::getFps() {
	updateFps();
	return mCurrentFps;
}
