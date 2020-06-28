#ifndef INCLUDE_UTIL_OBJANIMATION_H_
#define INCLUDE_UTIL_OBJANIMATION_H_

#include <vector>
#include <math.h>
#include "ModelOBJRenderer.h"

class OBJAnimation {

	Uint32 mStartTime {0};
	Uint32 mCurrentFrame {0};
	float mFrameTime;
	std::vector<std::shared_ptr<ModelOBJRenderer>> mFrames;

	void update() noexcept;
public:
	OBJAnimation(float frameTime):
		mFrameTime(frameTime)
	{}

	void addFrames(const std::string& filePrefix, const std::string& fileSuffix, unsigned int startFrame, unsigned int endFrame);

	void start() noexcept;
	void stop() noexcept;

	void render(const ICamera* camera, const ISky *sky, const IShadowMap* shadowMap, const Matrix4x4 m4x4, bool opaque);
};

//-----------------------------------------------------------------------------

inline void OBJAnimation::start() noexcept
{ mStartTime = SDL_GetTicks(); }

inline void OBJAnimation::stop() noexcept
{ mStartTime = 0; }


#endif
