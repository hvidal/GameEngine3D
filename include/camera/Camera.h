#ifndef CAMERA_H_
#define CAMERA_H_

#include "LinearMath/btVector3.h"
#include "../app/Interfaces.h"
#include "../util/Log.h"
#include "../util/math/Matrix4x4.h"


class Camera: public ICamera {

protected:
	unsigned long mVersion;
	float mFovY; // Vertical angle
	float mZnear; // The near clipping distance
	float mZfar; // The far clipping distance
	std::weak_ptr<IWindow> mWindow;

	btVector3 mPosition;
	btVector3 mTargetPosition;
	btVector3 mUp;
	float mViewAngle;
	bool mPaused;

	Matrix4x4 mViewMatrix;
	Matrix4x4 mProjectionMatrix;

	void writeVars(ISerializer* serializer) const;
	void readVars(ISerializer* serializer);
public:
	Camera(std::weak_ptr<IWindow> window, float fovY = 45.f, float nearPlane = 1.f, float farPlane = 100000.f);
	virtual ~Camera() { Log::debug("Deleting Camera"); }

	virtual unsigned long getVersion() const noexcept override;

	virtual const btVector3& getPosition() const noexcept override;
	void setPosition(btVector3 position) noexcept;

	const btVector3& getTargetPosition() const noexcept;
	void setTargetPosition(btVector3 target) noexcept;

	virtual const btVector3& getUp() const noexcept override;
	void setUp(btVector3 up) noexcept;

	virtual btVector3 getRight() const noexcept override;

	virtual btVector3 getDirection() const noexcept override;
	virtual btVector3 getDirectionUnit() const noexcept override;

	void setFovY(float v) noexcept;
	float getFovY() const noexcept;

	void setZnear(float v) noexcept;
	float getZnear() const noexcept;

	void setZfar(float v) noexcept;
	float getZfar() const noexcept;

	virtual bool isPaused() const noexcept override;
	virtual void setPause(bool) noexcept override;

	virtual bool isVisible(const btVector3& point) const noexcept override;

	virtual void updateControls() {}
	virtual void update() override;
	virtual void windowResized() override;
	virtual btVector3 getPointAt(int x, int y) const noexcept override;
	virtual bool isInFront(const btVector3& point) const noexcept override;

	virtual btVector3 getRayTo(int x, int y) const noexcept override;
	virtual void setViewport() const override;

	virtual void setMatrices(IShader* shader) const override;
	virtual void copyFrom(const ICamera*) noexcept override;
};

//-----------------------------------------------------------------------------

inline unsigned long Camera::getVersion() const noexcept
{ return mVersion; }

inline const btVector3& Camera::getPosition() const noexcept
{ return mPosition; }

inline void Camera::setPosition(btVector3 position) noexcept
{ mPosition = position; }

inline const btVector3& Camera::getTargetPosition() const noexcept
{ return mTargetPosition; }

inline void Camera::setTargetPosition(btVector3 target) noexcept
{ mTargetPosition = target; }

inline const btVector3& Camera::getUp() const noexcept
{ return mUp; }

inline void Camera::setUp(btVector3 up) noexcept
{ mUp = up; }

inline btVector3 Camera::getRight() const noexcept
{ return getDirection().cross(mUp); }

inline btVector3 Camera::getDirection() const noexcept
{ return mTargetPosition - mPosition; }

inline btVector3 Camera::getDirectionUnit() const noexcept
{ return getDirection().normalized(); }

inline void Camera::setFovY(float v) noexcept
{ mFovY = v; }

inline float Camera::getFovY() const noexcept
{ return mFovY; }

inline void Camera::setZnear(float v) noexcept
{ mZnear = v; }

inline float Camera::getZnear() const noexcept
{ return mZnear; }

inline void Camera::setZfar(float v) noexcept
{ mZfar = v; }

inline float Camera::getZfar() const noexcept
{ return mZfar; }

inline bool Camera::isPaused() const noexcept
{ return mPaused; }

inline void Camera::setPause(bool paused) noexcept
{ mPaused = paused; }

inline bool Camera::isInFront(const btVector3& point) const noexcept
{ return (point - mPosition).dot(getDirection()) > 0.f; }

inline void Camera::setViewport() const
{
	auto window = mWindow.lock();
	glViewport(0, 0, window->getWidth(), window->getHeight());
}


#endif
