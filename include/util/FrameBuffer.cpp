#include "FrameBuffer.h"


FrameBuffer::FrameBuffer() {
	glGenFramebuffers(1, &mFboId);
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
}


FrameBuffer::~FrameBuffer() {
	Log::debug("Deleting FrameBuffer");
	glDeleteFramebuffers(1, &mFboId);
}


void FrameBuffer::check() const {
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		Log::error("Framebuffer failed - exiting");
		exit(-1);
	}
}


void FrameBuffer::startColor(const ITexture* texture) const {
	glViewport(0, 0, texture->getWidth(), texture->getHeight());
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->getID(), 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	check();
}


void FrameBuffer::startDepth(const ITexture* texture) const {
	glViewport(0, 0, texture->getWidth(), texture->getHeight());
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->getID(), 0);
	glClear(GL_DEPTH_BUFFER_BIT);
	check();
}


void FrameBuffer::startColorAndDepth(const ITexture* colorTexture, const ITexture* depthTexture) const {
	glViewport(0, 0, colorTexture->getWidth(), colorTexture->getHeight());
	glBindFramebuffer(GL_FRAMEBUFFER, mFboId);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture->getID(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture->getID(), 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	check();
}


void FrameBuffer::end() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}