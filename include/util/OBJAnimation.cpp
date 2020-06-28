#include "OBJAnimation.h"


void OBJAnimation::addFrames(const std::string& filePrefix, const std::string& fileSuffix, unsigned int startFrame, unsigned int endFrame) {
	for (auto i = startFrame; i <= endFrame; ++i) {
		const std::string filename = filePrefix + (i < 10? "0" : "") + std::to_string(i) + fileSuffix;
		mFrames.push_back(ModelOBJRendererCache::get(filename));
	}
}


void OBJAnimation::update() noexcept {
	if (mStartTime) {
		const Uint32 now = SDL_GetTicks();
		const Uint32 timeSinceStart = now - mStartTime;
		Uint32 framesSinceStart = static_cast<Uint32>(floor(timeSinceStart / mFrameTime));
		Uint32 frame = framesSinceStart % mFrames.size();
		mCurrentFrame = frame;
	}
}


void OBJAnimation::render(const ICamera *camera, const ISky *sky, const IShadowMap *shadowMap, const Matrix4x4 m4x4, bool opaque) {
	update();
	mFrames[mCurrentFrame]->render(camera, sky, shadowMap, m4x4, opaque);
}