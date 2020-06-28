#ifndef RENDERER2D_H
#define RENDERER2D_H

#include <SDL2_ttf/SDL_ttf.h>
#include <unordered_map>
#include <map>
#include "../app/Interfaces.h"
#include "../util/math/Matrix4x4.h"


class Renderer2d: public IRenderer2d {

	Matrix4x4 mProjectionMatrix;

	std::weak_ptr<IWindow> mWindow;
	std::unordered_map<std::string, std::unique_ptr<TTF_Font, std::function<void(TTF_Font*)>>> mFonts;

	std::unique_ptr<ITexture> mCursorTexture;
	std::unique_ptr<IShader> mShader;

	std::map<unsigned int, std::unique_ptr<ITexture>> mTextureMap;

	void buildCursor();
	const int specX(const std::string& hspec, unsigned int imageWidth);
	const int specY(const std::string& vspec, unsigned int imageHeight);
	const TTF_Font* getFont(const std::string& fontFile, unsigned int fontSize);
	ITexture* getTexture(unsigned int labelId, int imageHeight);

	void renderImage(const ITexture* img, float x, float y, bool flipX, bool flipY);
public:
	static const std::string& SERIALIZE_ID;

	Renderer2d(std::weak_ptr<IWindow> window);
	virtual ~Renderer2d();

	virtual void begin() override;
	virtual void end() override;

	virtual void renderText(unsigned int labelId, const std::string& text, const SDL_Color& color, const int x, const int y, unsigned int fontSize) override;
 	virtual void renderText(unsigned int labelId, const std::string& text, const SDL_Color& color, const std::string& hspec, const std::string& vspec, unsigned int fontSize) override;
	virtual void renderImage(const ITexture* img, float x, float y) override;
	virtual void renderImageFlipX(const ITexture* img, float x, float y) override;
	virtual void renderImageFlipY(const ITexture* img, float x, float y) override;
	virtual void renderImageFlipXY(const ITexture* img, float x, float y) override;
	virtual void renderCursorAt(const int x, const int y) override;

	virtual void write(ISerializer *serializer) const override;
	virtual const std::string& serializeID() const noexcept override;
	static std::pair<std::string,Factory> factory();
};

#endif

