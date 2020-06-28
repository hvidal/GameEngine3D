#ifndef GAMEDEV3D_ISERIALIZATION_H
#define GAMEDEV3D_ISERIALIZATION_H

#include <functional>
#include <string>
#include <vector>
#include "btBulletDynamicsCommon.h"
#include "../util/Log.h"
#include "../util/math/Matrix4x4.h"
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_keycode.h>
#include <unordered_map>
#include <SDL2/SDL_pixels.h>
#include <OpenGL/gl3.h>
#include <SDL2/SDL_surface.h>


class Factory;
class IWindow;


enum class GameMode {
	RUNNING,
	PAUSED,
	EDITING
};

enum class DebugCode {
	NONE,
	COLLISION_SHAPE,
	WATER_REFLECTION,
	NO_SHADOW,
	SIMPLIFIED
};

typedef struct {
	GameMode mode = GameMode::RUNNING;
	int mouseX = 0;
	int mouseY = 0;
	btVector3 mouse3d = { -BT_LARGE_FLOAT, -BT_LARGE_FLOAT, -BT_LARGE_FLOAT };
	bool isLeftMouseDown = false;
	bool isRightMouseDown = false;
	bool isFreeFly = false;
	DebugCode debugCode = DebugCode::NONE;
} GameState;

//-----------------------------------------------------------------------------

class ISerializer {
public:
	virtual ~ISerializer() {};

	virtual void open(bool createFile = false) = 0;
	virtual void close() = 0;

	virtual void writeBegin(const std::string&, unsigned long) = 0;
	virtual void write(const btVector3&) = 0;
	virtual void write(const std::string&) = 0;
	virtual void write(bool) = 0;
	virtual void write(int) = 0;
	virtual void write(unsigned int) = 0;
	virtual void write(unsigned long) = 0;
	virtual void write(float) = 0;
	virtual void write(const std::vector<unsigned int>&) = 0;

	virtual bool readBegin(std::string&, unsigned long&) = 0;
	virtual void read(btVector3&) = 0;
	virtual void read(std::string&) = 0;
	virtual void read(bool&) = 0;
	virtual void read(int&) = 0;
	virtual void read(unsigned int&) = 0;
	virtual void read(unsigned long&) = 0;
	virtual void read(float&) = 0;
	virtual void read(std::vector<unsigned int>&) = 0;

	virtual void addFactory(std::pair<std::string,Factory>) = 0;
	virtual const Factory& getFactory(const std::string&) const = 0;
};

//-----------------------------------------------------------------------------

class ISerializable {
private:
	unsigned long mObjectId;
protected:
	void setObjectId(unsigned long id) { mObjectId = id; }
public:
	ISerializable() {
		static unsigned long serializableID = 0;
		mObjectId = ++serializableID;
	}
	virtual ~ISerializable() {}

	unsigned long getObjectId() const noexcept { return mObjectId; }

	virtual void write(ISerializer*) const = 0;
	virtual const std::string& serializeID() const noexcept = 0;
};

//-----------------------------------------------------------------------------

class Factory {
	std::function<std::shared_ptr<ISerializable>(unsigned long,ISerializer*,std::shared_ptr<IWindow>)> mFunction;

public:
	Factory(std::function<std::shared_ptr<ISerializable>(unsigned long,ISerializer*,std::shared_ptr<IWindow>)> function):
		mFunction(function)
	{}

	std::shared_ptr<ISerializable> create(unsigned long, ISerializer* serializer, std::shared_ptr<IWindow> window) const;
};

inline std::shared_ptr<ISerializable> Factory::create(unsigned long objectId, ISerializer* serializer, std::shared_ptr<IWindow> window) const {
	return mFunction(objectId, serializer, window);
}

//-----------------------------------------------------------------------------

class ITexture: public ISerializable {
public:
	virtual ~ITexture(){}

	virtual GLuint getSlot() const noexcept = 0;
	virtual GLuint getID() const noexcept = 0;
	virtual unsigned int getWidth() const noexcept = 0;
	virtual unsigned int getHeight() const noexcept = 0;

	virtual void bind() const = 0;
	virtual ITexture* mipmap() = 0;
	virtual ITexture* image() = 0;
	virtual ITexture* update(SDL_Surface*) = 0;
	virtual ITexture* linear() = 0;
	virtual ITexture* nearest() = 0;
	virtual ITexture* wrap(GLint value) = 0;
	virtual ITexture* repeat() = 0;
	virtual ITexture* repeatMirrored() = 0;
	virtual ITexture* clampToBorder() = 0;
	virtual ITexture* clampToEdge() = 0;
	virtual ITexture* scaleFactor(float) = 0;
	virtual float getScaleFactor() = 0;
	virtual void setFromSurface(const SDL_Surface* img) = 0;

	static void unbind() {
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
	}
};

//-----------------------------------------------------------------------------

class IShader {
public:
	virtual ~IShader(){};

	virtual void run() const = 0;
	virtual void link() const = 0;
	virtual void bindAttribute(GLuint index, const GLchar *name) const = 0;
	virtual void set(const std::string& param, const ITexture* texture) = 0;
	virtual void set(const std::string& param, const btVector3& vector) = 0;
	virtual void set(const std::string& param, float value) = 0;
	virtual void set(const std::string& param, int value) = 0;
	virtual void set(const std::string& param, float v1, float v2) = 0;
	virtual void set(const std::string& param, const Matrix4x4& matrix) = 0;
	virtual void set4f(const std::string& param, float* value) = 0;
	static void stop() { glUseProgram(0); }
};

//-----------------------------------------------------------------------------

class IRenderer2d: public ISerializable {
public:
	virtual ~IRenderer2d(){}

	virtual void begin() = 0;
	virtual void end() = 0;
	virtual void renderText(unsigned int labelId, const std::string& text, const SDL_Color& color, const int x, const int y, unsigned int fontSize) = 0;
	virtual void renderText(unsigned int labelId, const std::string& text, const SDL_Color& color, const std::string& hspec, const std::string& vspec, unsigned int fontSize) = 0;
	virtual void renderImage(const ITexture* img, float x, float y) = 0;
	virtual void renderImageFlipX(const ITexture* img, float x, float y) = 0;
	virtual void renderImageFlipY(const ITexture* img, float x, float y) = 0;
	virtual void renderImageFlipXY(const ITexture* img, float x, float y) = 0;
	virtual void renderCursorAt(const int x, const int y) = 0;
};

//-----------------------------------------------------------------------------

class IShadowMap: public ISerializable {
public:
	virtual ~IShadowMap(){}

	virtual void start(const btVector3& lightPosition, const btVector3& targetPosition, const btVector3& up) = 0;
	virtual void end() = 0;
	virtual bool isRendering() const = 0;
	virtual void setMatrices(IShader* shader) const = 0;
	virtual void setVars(IShader* shader) const = 0;
};

//-----------------------------------------------------------------------------

class ICamera: public ISerializable {
public:
	virtual ~ICamera(){}

	virtual unsigned long getVersion() const noexcept = 0;
	virtual const btVector3& getPosition() const noexcept = 0;
	virtual const btVector3& getUp() const noexcept = 0;
	virtual btVector3 getRight() const noexcept = 0;
	virtual btVector3 getDirection() const noexcept = 0;
	virtual btVector3 getDirectionUnit() const noexcept = 0;
	virtual btVector3 getRayTo(int x, int y) const noexcept = 0;
	virtual void setViewport() const = 0;
	virtual bool isVisible(const btVector3& point) const noexcept = 0;
	virtual btVector3 getPointAt(int x, int y) const noexcept = 0;
	virtual void update() = 0;
	virtual void windowResized() = 0;
	virtual bool isPaused() const noexcept = 0;
	virtual void setPause(bool) noexcept = 0;
	virtual bool isInFront(const btVector3& point) const noexcept = 0;
	virtual void setMatrices(IShader* shader) const = 0;
	virtual void copyFrom(const ICamera*) noexcept = 0;
};

//-----------------------------------------------------------------------------

class ISky {
public:
	virtual ~ISky(){}

	virtual float getAmbientLight() const noexcept = 0;
	virtual void setAmbientLight(float ambient) noexcept = 0;

	virtual float getDiffuseLight() const noexcept = 0;
	virtual void setDiffuseLight(float diffuse) noexcept = 0;

	virtual float getSpecularLight() const noexcept = 0;
	virtual void setSpecularLight(float specular) noexcept = 0;

	virtual const btVector3& getSunPosition() const noexcept = 0;
	virtual void setSunPosition(btVector3 sunPos) noexcept = 0;

	virtual void setVars(IShader* shader, const ICamera* camera) const = 0;

	virtual void debug() const = 0;
};

//-----------------------------------------------------------------------------

class ISurfaceReflection: public ISerializable {
public:
	virtual ~ISurfaceReflection(){}

	virtual const ITexture* getColorTexture() const = 0;
	virtual bool isRendering() const = 0;

	virtual void begin() = 0;
	virtual void end() = 0;

	virtual void setVars(IShader* shader, const ICamera* camera) const = 0;
	virtual void setReflectionTexture(IShader* shader) const = 0;
};

//-----------------------------------------------------------------------------

class ISceneObject: public ISerializable {
public:
	virtual ~ISceneObject(){}

	using CommandMap = std::unordered_map<std::string, std::function<void(const std::string&)>>;

	/** returns an empty map by default */
	virtual CommandMap getCommands()
	{ return CommandMap(); }

	virtual void initPhysics(btTransform transform) {};
	virtual void renderOpaque(const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) = 0;
	virtual void renderTranslucent(const ICamera* camera, const ISky* sky, const IShadowMap* shadowMap, const ISurfaceReflection* surfaceReflection, const GameState& gameState) {};
	virtual void render2d(IRenderer2d* renderer2d) {};
	virtual void updateSimulation() {}
	virtual void reset() {};

	// If a camera is focusing on this object, respect these numbers
	virtual float getCameraHeight() const noexcept = 0;
	virtual float getMinCameraDistance() const noexcept = 0;
	virtual float getMaxCameraDistance() const noexcept = 0;

	virtual Matrix4x4 getMatrix4x4() const = 0;
};

//-----------------------------------------------------------------------------

class IKeyboardListener
{
public:
	virtual ~IKeyboardListener() {}

	virtual void keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) = 0;
	virtual void keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) {};
	virtual void update(const ICamera* camera) {}
	virtual void reset() {}
};

//-----------------------------------------------------------------------------

class IMouseListener
{
public:
	enum class ButtonCode {
		LEFT_BUTTON = 1,
		MIDDLE_BUTTON = 2,
		RIGHT_BUTTON = 3,
		WHEEL_UP = 3,
		WHEEL_DOWN = 4
	};

	virtual ~IMouseListener() {}

	virtual void mouseDown(Uint8 button, int x, int y, const GameState& gameState) {};
	virtual void mouseUp(Uint8 button, int x, int y, const GameState& gameState) {};
	virtual void mouseMove(int x, int y, const GameState& gameState) {};
	virtual void reset() {}
};

//-----------------------------------------------------------------------------

class IGameScene {
public:
	virtual ~IGameScene(){}

	virtual std::shared_ptr<btDynamicsWorld> getDynamicsWorld() const noexcept = 0;
	virtual void setGameCamera(std::shared_ptr<ICamera>) noexcept = 0;
	virtual void setFlyCamera(std::shared_ptr<ICamera>) noexcept = 0;
	virtual void setSky(std::shared_ptr<ISky>) noexcept = 0;
	virtual void setShadowMap(std::shared_ptr<IShadowMap>) noexcept = 0;
	virtual void setSurfaceReflection(std::shared_ptr<ISurfaceReflection>) noexcept = 0;
	virtual void setRenderer2d(std::shared_ptr<IRenderer2d>) noexcept = 0;
	virtual void enableSerialization(std::shared_ptr<ISerializer>) = 0;
	virtual void addSerializable(std::shared_ptr<ISerializable>) noexcept = 0;
	virtual std::shared_ptr<ISerializable> getSerializable(const std::string& serializeID) const = 0;
	virtual std::shared_ptr<ISceneObject> getByObjectId(unsigned long) const = 0;
	virtual void addSceneObject(std::shared_ptr<ISceneObject>) = 0;
	virtual void addMouseListener(std::shared_ptr<IMouseListener>) noexcept = 0;
	virtual void addKeyboardListener(std::shared_ptr<IKeyboardListener>) noexcept = 0;
	virtual void addCommands(ISceneObject::CommandMap commands) = 0;
	virtual ICamera* getActiveCamera() const noexcept = 0;

	virtual void keyDown(SDL_Keycode key, bool shift, bool ctrl, bool alt) = 0;
	virtual void keyUp(SDL_Keycode key, bool shift, bool ctrl, bool alt) = 0;
	virtual void mouseDown(Uint8 button, int x, int y) = 0;
	virtual void mouseUp(Uint8 button, int x, int y) = 0;
	virtual void mouseMove(int x, int y) = 0;

	virtual void moveAndDisplay() = 0;
};

//-----------------------------------------------------------------------------

class IWindow {
public:
	virtual ~IWindow() {}

	virtual unsigned int getWidth() const noexcept = 0;
	virtual unsigned int getHeight() const noexcept = 0;
	virtual float getAspectRatio() const noexcept = 0;
	virtual void resize(unsigned int width, unsigned int height) = 0;
	virtual void centerMouseCursor() const = 0;
	virtual void setFullScreen() = 0;
	virtual void addResizeListener(std::function<void(int, int)>) = 0;

	virtual void setGameScene(std::shared_ptr<IGameScene>) = 0;
	virtual std::shared_ptr<IGameScene> getGameScene() const = 0;

	virtual void loop() = 0;
};


#endif
