#include "Texture.h"
#include "IoUtils.h"

const std::string& Texture::SERIALIZE_ID = "Texture";


Texture::Texture(GLuint slot):
mSlot(slot)
{}


Texture::Texture(GLuint slot, const std::string& filename):
	mSlot(slot)
{
	glGenTextures(1, &mTextureID);

	static auto deleter = [](SDL_Surface* img) {
		SDL_FreeSurface(img);
	};

	auto img = std::unique_ptr<SDL_Surface, decltype(deleter)>(IMG_Load(filename.c_str()), deleter);
	if (!img) {
		Log::error("Unable to load texture: %s", filename.c_str());
		throw std::runtime_error(std::string("Unable to load texture: ") + filename);
	}
	setFromSurface(img.get());
	mFileName = filename;
}


Texture::Texture(GLuint slot, const SDL_Surface* surface):
	mSlot(slot)
{
	glGenTextures(1, &mTextureID);
	setFromSurface(surface);
}


Texture::Texture(GLuint slot, int colorCount, GLenum textureFormat, unsigned int width, unsigned int height, std::vector<Uint32>&& pixels):
	mSlot(slot),
	mColorCount(colorCount),
	mTextureFormat(textureFormat),
	mWidth(width),
	mHeight(height),
	mPixels(std::move(pixels))
{
	glGenTextures(1, &mTextureID);
}


Texture::Texture(GLuint slot, int colorCount, unsigned int width, unsigned int height, std::function<void(Uint32*,const SDL_PixelFormat*)> painter):
	mSlot(slot),
	mColorCount(colorCount),
	mWidth(width),
	mHeight(height)
{
	Uint32 rmask, gmask, bmask, amask;
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif
	glGenTextures(1, &mTextureID);
	SDL_Surface* img = SDL_CreateRGBSurface(0, mWidth, mHeight, 8 * mColorCount, rmask, gmask, bmask, amask);
	SDL_FillRect(img, NULL, SDL_MapRGBA(img->format, 0, 0, 0, 0));
	painter((Uint32*) img->pixels, img->format);
	setFromSurface(img);
	SDL_FreeSurface(img);
}


Texture::~Texture() {
	Log::debug("Deleting Texture %u", mSlot);
	glDeleteTextures(1, (const GLuint*) &mTextureID);
	if (mPBO != 0)
		glDeleteBuffers(1, &mPBO);
}


void Texture::loadPixels(const void *pixels) {
	mPixels.clear();
	if (pixels != nullptr) {
		int size = mWidth * mHeight;
		mPixels.reserve(size);
		const Uint32* pixels0 = static_cast<const Uint32*>(pixels);
		for (auto i = 0; i < size; ++i) {
			mPixels[i] = pixels0[i];
		}
	}
}


void Texture::setFromSurface(const SDL_Surface* surface) {
	mFileName.clear();
	// get the number of channels in the SDL surface
	mColorCount = surface->format->BytesPerPixel;
	if (mColorCount == 4) // contains an alpha channel
		mTextureFormat = surface->format->Rmask == 0x000000ff? GL_RGBA : GL_BGRA;
	else if (mColorCount == 3) // no alpha channel
		mTextureFormat = surface->format->Rmask == 0x000000ff? GL_RGB : GL_BGR;
	else
		throw std::runtime_error("Error: image is not truecolor");
	mWidth = (unsigned int) surface->w;
	mHeight = (unsigned int) surface->h;

	loadPixels(surface->pixels);
}


void Texture::bind() const {
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0 + mSlot);
	glBindTexture(GL_TEXTURE_2D, mTextureID);
}


ITexture* Texture::mipmap() {
	bind();
	image();
	glGenerateMipmap(GL_TEXTURE_2D);
	return this;
}


ITexture* Texture::image() {
	bind();
	glTexImage2D(GL_TEXTURE_2D, 0, mColorCount, mWidth, mHeight, 0, mTextureFormat, GL_UNSIGNED_BYTE, &mPixels[0]);
	return this;
}


ITexture *Texture::update(SDL_Surface *surface) {
	const unsigned int sizeTexture = mWidth * mHeight * mColorCount;
	if (mPBO == 0) {
		glGenBuffers(1, &mPBO);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPBO);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeTexture, 0, GL_STREAM_DRAW);
	}

	bind();
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mPBO);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, mTextureFormat, GL_UNSIGNED_BYTE, 0);

	glBufferData(GL_PIXEL_UNPACK_BUFFER, sizeTexture, 0, GL_STREAM_DRAW);
	Uint32* ptr = static_cast<Uint32*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));
	if (ptr) {
		const Uint32* pixels0 = static_cast<const Uint32*>(surface->pixels);
		for (int h = 0; h < surface->h; ++h ) {
			for (int w = 0; w < surface->w; ++w) {
				*ptr = pixels0[w + h * surface->w];
				ptr++;
			}
			// clean up the rest of the line
			for (int w = surface->w; w < mWidth; ++w) {
				*ptr = 0;
				ptr++;
			}
		}
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER); // release pointer to mapping buffer
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return this;
}


ITexture* Texture::linear() {
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	return this;
}


ITexture* Texture::nearest() {
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return this;
}


ITexture* Texture::wrap(GLint value){
	bind();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, value);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, value);
	return this;
}


ITexture* Texture::repeat() {
	return wrap(GL_REPEAT);
}


ITexture* Texture::repeatMirrored() {
	return wrap(GL_MIRRORED_REPEAT);
}


ITexture* Texture::clampToBorder() {
	return wrap(GL_CLAMP_TO_BORDER);
}


ITexture* Texture::clampToEdge() {
	return wrap(GL_CLAMP_TO_EDGE);
}


ITexture* Texture::scaleFactor(float factor) {
	mScaleFactor = factor;
	return this;
}


float Texture::getScaleFactor() {
	return mScaleFactor;
}


void Texture::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	serializer->write(mSlot);
	serializer->write(mScaleFactor);
	serializer->write(mFileName);

	if (mFileName.length() == 0) {
		serializer->write(mTextureFormat);
		serializer->write(mColorCount);
		serializer->write(mWidth);
		serializer->write(mHeight);
		serializer->write(mPixels);
	}
}


std::pair<std::string,Factory> Texture::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		GLuint slot;
		std::string filename;
		float scaleFactor;
		serializer->read(slot);
		serializer->read(scaleFactor);
		serializer->read(filename);

		std::shared_ptr<Texture> o;
		Log::debug("Loading Texture: %s", filename.c_str());
		if (filename.length() == 0) {
			GLenum textureFormat;
			GLint colorCount;
			unsigned int width, height;
			std::vector<Uint32> pixels0;

			serializer->read(textureFormat);
			serializer->read(colorCount);
			serializer->read(width);
			serializer->read(height);
			serializer->read(pixels0);

			o = std::make_shared<Texture>(slot, colorCount, textureFormat, width, height, std::move(pixels0));
		} else {
			o = std::make_shared<Texture>(slot, filename);
		}
		o->setObjectId(objectId);
		o->scaleFactor(scaleFactor);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}


const std::string& Texture::serializeID() const noexcept {
	return SERIALIZE_ID;
}








