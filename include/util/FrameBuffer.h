#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_

#include "../app/Interfaces.h"


class FrameBuffer
{
	GLuint mFboId;

public:
	FrameBuffer();
	virtual ~FrameBuffer();

	void check() const;
	void startColor(const ITexture* texture) const;
	void startDepth(const ITexture* texture) const;
	void startColorAndDepth(const ITexture* colorTexture, const ITexture* depthTexture) const;
	static void end();
};


#endif