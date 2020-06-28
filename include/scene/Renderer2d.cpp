#include "Renderer2d.h"
#include "../util/IoUtils.h"
#include "../util/Shader.h"
#include "../util/Texture.h"

const static std::string DEFAULT_FONT { "/font/SourceSansPro-Regular.ttf" };
const static GLuint gSlot = 0;

static constexpr const char* _vs[] = {
	"attribute vec2 vertex;"
	"attribute vec2 uv;"
	"uniform mat4 P;"
	"varying vec2 _uv;"
	"void main(void) {"
		"_uv = uv;"
		"gl_Position = P * vec4(vertex, 0.0, 1.0);"
	"}"
};

static constexpr const char* _fs[] = {
	"uniform sampler2D texture;"
	"varying vec2 _uv;"
	"void main() {"
		"gl_FragColor = texture2D(texture, _uv);"
	"}"
};


const std::string& Renderer2d::SERIALIZE_ID = "Renderer2d";

//-----------------------------------------------------------------------------

Renderer2d::Renderer2d(std::weak_ptr<IWindow> window):
	mWindow{window}
{
	if (TTF_Init() == -1)
		throw std::runtime_error("TTF Init Failed");

	buildCursor();
	mShader = std::make_unique<Shader>(_vs, _fs);
	mShader->bindAttribute(0, "vertex");
	mShader->bindAttribute(1, "uv");
	mShader->link();

	auto _window = mWindow.lock();

	// Add listener to update projection matrix when window resizes
	_window->addResizeListener([this](int w, int h){
		mProjectionMatrix.ortho(0, w, 0, h, -1.f, 1.f);
	});
	mProjectionMatrix.ortho(0, _window->getWidth(), 0, _window->getHeight(), -1.f, 1.f);
}


Renderer2d::~Renderer2d() {
	Log::debug("Deleting Renderer2d");

	// Delete fonts before calling TTF_Quit
	mFonts.clear();
	TTF_Quit();
}


ITexture* Renderer2d::getTexture(unsigned int labelId, int imageHeight) {
	if (mTextureMap.count(labelId) == 0) {
		static constexpr const unsigned int width = 300;
		mTextureMap[labelId] = std::make_unique<Texture>(gSlot, 4, GL_BGRA, width, imageHeight, std::vector<Uint32>(width * imageHeight));
		mTextureMap[labelId]->linear()->repeat()->image()->unbind();
	}
	return mTextureMap[labelId].get();
}


void Renderer2d::buildCursor() {
	const int size = 21;
	std::function<void(Uint32*,const SDL_PixelFormat*)> painter = [&size](Uint32* pixels, const SDL_PixelFormat* format) {
		const Uint32 white = SDL_MapRGB(format, 240, 240, 240);
		const Uint32 black = SDL_MapRGB(format, 15, 15, 15);
		const int middle = size / 2;
		for (int i = 0; i < size; ++i) {
			Uint32 color = i % 2 == 0? white : black;
			// middle column
			int pos = i * size + middle;
			pixels[pos] = color;

			// middle row
			pos = middle * size + i;
			pixels[pos] = color;
		}
	};
	mCursorTexture = std::make_unique<Texture>(gSlot, 4, size, size, painter);
	mCursorTexture->mipmap()->repeat()->unbind();
}


const int Renderer2d::specX(const std::string &hspec, unsigned int imageWidth) {
	const int windowWidth = mWindow.lock()->getWidth();
	float x;
	if (hspec == "center") {
		x = windowWidth / 2.f - imageWidth / 2.f;
	} else {
		auto pos = hspec.find_last_of('=');
		std::string value = hspec.substr(pos + 1);
		float v = (float) ::atof(value.c_str());
		std::string prefix { hspec.substr(0, pos) };
		if (prefix == "left") {
			x = v;
		} else if (prefix == "right") {
			x = windowWidth - v;
		} else
			throw std::runtime_error("Invalid horizontal spec: " + hspec);
	}
	return static_cast<int>(x);
}


const int Renderer2d::specY(const std::string &vspec, unsigned int imageHeight) {
	const int windowHeight = mWindow.lock()->getHeight();
	float y;
	if (vspec == "middle") {
		y = windowHeight / 2.f - imageHeight / 2.f;
	} else {
		auto pos = vspec.find_last_of('=');
		std::string value = vspec.substr(pos + 1);
		float v = (float) ::atof(value.c_str());
		std::string prefix { vspec.substr(0, pos) };
		if (prefix == "top") {
			y = windowHeight - v - imageHeight;
		} else if (prefix == "bottom") {
			y = v;
		} else
			throw std::runtime_error("Invalid vertical spec: " + vspec);
	}
	return static_cast<int>(y);
}


const TTF_Font* Renderer2d::getFont(const std::string& fontFile, unsigned int fontSize) {
	std::string key = fontFile + std::to_string(fontSize);
	if (mFonts.count(key) == 0) {
		// Use std::function because the deleter type is part of the pointer type
		std::function<void(TTF_Font*)> deleter = [](TTF_Font* font){
			Log::debug("Deleting font %u", font);
			TTF_CloseFont(font);
		};
		auto font = std::unique_ptr<TTF_Font, std::function<void(TTF_Font*)>>(TTF_OpenFont(IoUtils::resource(fontFile).c_str(), fontSize), deleter);
		if (font.get() == nullptr)
			throw std::runtime_error("Failed to load font: " + fontFile + " | " + TTF_GetError());
		TTF_Font* f = font.get();
		mFonts[key] = std::move(font);
		return f;
	} else
		return mFonts[key].get();
}


void Renderer2d::begin() {
	// unbind buffer data, if any
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer2d::end() {
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}


void Renderer2d::renderCursorAt(const int x, const int y) {
	// We have to revert the Y coord because (0,0) is at the bottom left
	renderImage(mCursorTexture.get(), x - mCursorTexture->getWidth()/2, mWindow.lock()->getHeight() - y - mCursorTexture->getHeight()/2);
}


void Renderer2d::renderText(unsigned int labelId, const std::string &text, const SDL_Color &color, const int x, const int y, unsigned int fontSize) {
	const TTF_Font* font = getFont(DEFAULT_FONT, fontSize);
	SDL_Surface *surface = TTF_RenderUTF8_Blended(const_cast<TTF_Font*>(font), text.c_str(), color);

	ITexture* pTexture = getTexture(labelId, surface->h);
	pTexture->update(surface);
	renderImage(pTexture, x, y);

	SDL_FreeSurface(surface);
}


void Renderer2d::renderText(unsigned int labelId, const std::string& text, const SDL_Color& color, const std::string &hspec, const std::string &vspec, unsigned int fontSize) {
	const TTF_Font* font = getFont(DEFAULT_FONT, fontSize);
	SDL_Surface *surface = TTF_RenderUTF8_Blended(const_cast<TTF_Font*>(font), text.c_str(), color);

	ITexture* pTexture = getTexture(labelId, surface->h);
	pTexture->update(surface);
	SDL_FreeSurface(surface);

	float x = specX(hspec, surface->w);
	float y = specY(vspec, surface->h);

	renderImage(pTexture, x, y);
}


typedef struct {
	float st[2];
} Point2d;

static constexpr const unsigned int indices[4] = { 0, 1, 2, 3 };
static constexpr const Point2d texCoord[4] = { {0.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}, {1.f, 0.f} };
static constexpr const Point2d texCoord_flipY[4] = { {0.f, 0.f}, {0.f, -1.f}, {1.f, -1.f}, {1.f, 0.f} };
static constexpr const Point2d texCoord_flipX[4] = { {0.f, 0.f}, {0.f, 1.f}, {-1.f, 1.f}, {-1.f, 0.f} };
static constexpr const Point2d texCoord_flipXY[4] = { {0.f, 0.f}, {0.f, -1.f}, {-1.f, -1.f}, {-1.f, 0.f} };

void Renderer2d::renderImage(const ITexture* img, float x, float y, bool flipX, bool flipY) {
	const int w = img->getWidth();
	const int h = img->getHeight();

	const Point2d vertices[4] = { {x, h + y}, {x, y}, {w + x, y}, {w + x, h + y} };

	mShader->run();
	mShader->set("P", mProjectionMatrix);
	mShader->set("texture", img);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point2d), &vertices[0]);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Point2d), flipX && flipY? &texCoord_flipXY[0] : flipX? &texCoord_flipX[0] : flipY? &texCoord_flipY[0] : &texCoord[0]);

	glDrawElements(GL_QUADS, 4, GL_UNSIGNED_INT, &indices[0]);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);

	Shader::stop();
	Texture::unbind();
}


void Renderer2d::renderImage(const ITexture* img, float x, float y) {
	renderImage(img, x, y, false, false);
}


void Renderer2d::renderImageFlipX(const ITexture *img, float x, float y) {
	renderImage(img, x, y, true, false);
}


void Renderer2d::renderImageFlipY(const ITexture *img, float x, float y) {
	renderImage(img, x, y, false, true);
}


void Renderer2d::renderImageFlipXY(const ITexture *img, float x, float y) {
	renderImage(img, x, y, true, true);
}


void Renderer2d::write(ISerializer *serializer) const {
	serializer->writeBegin(serializeID(), getObjectId());
	// Nothing to serialize
}

std::pair<std::string,Factory> Renderer2d::factory() {
	auto factory = [](unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) {
		std::shared_ptr<Renderer2d> o = std::make_shared<Renderer2d>(window);
		o->setObjectId(objectId);

		IGameScene* gameScene = window->getGameScene().get();
		gameScene->setRenderer2d(o);
		gameScene->addSerializable(o);
		return o;
	};
	return std::make_pair(SERIALIZE_ID, Factory(factory));
}

const std::string& Renderer2d::serializeID() const noexcept {
	return SERIALIZE_ID;
}

