#ifndef TEXTURE_H_
#define TEXTURE_H_

#include <string>
#include <functional>
#include <SDL2_image/SDL_image.h>
#include "../app/Interfaces.h"


class Texture: public ITexture
{
	std::string mFileName;
	GLuint mSlot;
	GLuint mPBO {0};
	GLuint mTextureID;
	GLint mColorCount;
	GLenum mTextureFormat;
	float mScaleFactor {1.f};
	unsigned int mWidth;
	unsigned int mHeight;
	std::vector<Uint32> mPixels;

	void loadPixels(const void* pixels);
public:
	static const std::string& SERIALIZE_ID;

	Texture(GLuint slot);
	Texture(GLuint slot, const std::string& filename);
	Texture(GLuint slot, const SDL_Surface* surface);
	Texture(GLuint slot, int colorCount, GLenum textureFormat, unsigned int width, unsigned int height, std::vector<Uint32>&& pixels);
	Texture(GLuint slot, int colorCount, unsigned int width, unsigned int height, std::function<void(Uint32*,const SDL_PixelFormat*)> painter);
	~Texture();

	virtual GLuint getSlot() const noexcept override;
	virtual GLuint getID() const noexcept override;
	virtual unsigned int getWidth() const noexcept override;
	virtual unsigned int getHeight() const noexcept override;

	virtual void bind() const override;
	virtual ITexture* mipmap() override;
	virtual ITexture* image() override;
	virtual ITexture* update(SDL_Surface*) override;
	virtual ITexture* linear() override;
	virtual ITexture* nearest() override;
	virtual ITexture* wrap(GLint value) override;
	virtual ITexture* repeat() override;
	virtual ITexture* repeatMirrored() override;
	virtual ITexture* clampToBorder() override;
	virtual ITexture* clampToEdge() override;
	virtual ITexture* scaleFactor(float) override;
	virtual float getScaleFactor() override;
	virtual void setFromSurface(const SDL_Surface* img) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

//-----------------------------------------------------------------------------

inline GLuint Texture::getSlot() const noexcept
{ return mSlot; }

inline GLuint Texture::getID() const noexcept
{ return mTextureID; }

inline unsigned int Texture::getWidth() const noexcept
{ return mWidth; }

inline unsigned int Texture::getHeight() const noexcept
{ return mHeight; }


#endif