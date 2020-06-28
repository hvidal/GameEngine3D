#include "Camera.h"
#include "../util/GLUtils.h"


Camera::Camera(std::weak_ptr<IWindow> window, float fovY, float nearPlane, float farPlane) :
	mVersion(0),
	mWindow(std::move(window)),
	mFovY(fovY),
	mZnear(nearPlane),
	mZfar(farPlane),
	mPosition(0.f,0.f,0.f),
	mTargetPosition(0.f,0.f,0.f),
	mUp(0.f,1.f,0.f),
	mViewAngle(45.f),
	mPaused(false)
{}


static std::pair<float, double> cosAngleCache;

bool Camera::isVisible(const btVector3& point) const noexcept {
	if (cosAngleCache.first != mViewAngle)
		cosAngleCache = std::make_pair(mViewAngle, cos(mViewAngle * 0.01745329251994f));
	float dot = (point - mPosition).normalized().dot(getDirectionUnit());
	return dot >= cosAngleCache.second;
}


btVector3 Camera::getRayTo(int x, int y) const noexcept {
	btVector3 rayTo = getPointAt(x, y) - mPosition;
	return rayTo * mZfar;
}

static unsigned long gLastVersionUpdate = -1;

void Camera::update() {
	if (!mPaused)
		updateControls();

	if (gLastVersionUpdate == mVersion)
		return;

	mViewMatrix.lookAt(mPosition, mTargetPosition, mUp);
	mProjectionMatrix.perspective(mFovY, mWindow.lock()->getAspectRatio(), mZnear, mZfar);

	gLastVersionUpdate = mVersion;
}


void Camera::windowResized() {
	update();

	btVector3 toCorner = getPointAt(0, mWindow.lock()->getHeight()) - mPosition;
	toCorner.normalize();
	const btVector3& direction1 = getDirectionUnit();
	float angle = toCorner.angle(direction1) * 57.2957795f; // convert to degrees
	mViewAngle = angle + 2.5f;
	Log::debug("mViewAngle = %.2f", mViewAngle);
}


btVector3 Camera::getPointAt(int x, int y) const noexcept {
	int viewport[4];
	float winX, winY, winZ;
	double pos[3];

	glGetIntegerv(GL_VIEWPORT, viewport);

	winX = (float) x;
	winY = viewport[3] - (float) y;
	glReadPixels(winX, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ);
	glhUnProjectf(winX, winY, winZ, mViewMatrix.raw(), mProjectionMatrix.raw(), viewport, pos);
	return btVector3(pos[0], pos[1], pos[2]);
};


void Camera::writeVars(ISerializer* serializer) const {
	serializer->write(mFovY);
	serializer->write(mZnear);
	serializer->write(mZfar);
	serializer->write(mViewAngle);

	serializer->write(mPosition);
	serializer->write(mTargetPosition);
	serializer->write(mUp);
}


void Camera::readVars(ISerializer* serializer) {
	serializer->read(mFovY);
	serializer->read(mZnear);
	serializer->read(mZfar);
	serializer->read(mViewAngle);

	serializer->read(mPosition);
	serializer->read(mTargetPosition);
	serializer->read(mUp);
}


void Camera::setMatrices(IShader* shader) const {
	shader->set("V", mViewMatrix);
	shader->set("P", mProjectionMatrix);
}

void Camera::copyFrom(const ICamera* camera) noexcept {
	mPosition = camera->getPosition();
	mTargetPosition = mPosition + camera->getDirection();
	mUp = camera->getUp();
	mVersion++;
}


