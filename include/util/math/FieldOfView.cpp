#include "FieldOfView.h"


FieldOfView::FieldOfView(const btVector3& center, const btVector3& a, const btVector3 &b, const btVector3 &c, const btVector3 &d) noexcept:
	mCenter(center)
{
	const btVector3&& vA = a - mCenter;
	const btVector3&& vB = b - mCenter;
	const btVector3&& vC = c - mCenter;
	const btVector3&& vD = d - mCenter;

	// Find the normal vectors for each plane.
	// The dot product of the normal with another point should be positive
	// So we revert the normal if this is not the case
	mNormals[0] = vA.cross(vB);
	if (mNormals[0].dot(vC) < 0.0f)
		mNormals[0] = -mNormals[0];

	mNormals[1] = vB.cross(vC);
	if (mNormals[1].dot(vD) < 0.0f)
		mNormals[1] = -mNormals[1];

	mNormals[2] = vC.cross(vD);
	if (mNormals[2].dot(vA) < 0.0f)
		mNormals[2] = -mNormals[2];

	mNormals[3] = vD.cross(vA);
	if (mNormals[3].dot(vB) < 0.0f)
		mNormals[3] = -mNormals[3];
}

static constexpr const float LIMIT = -0.001f;

bool FieldOfView::isVisible(const btVector3& point) const noexcept {
	const btVector3&& v = point - mCenter;
	return
		mNormals[0].dot(v) >= LIMIT &&	// dotBA
		mNormals[1].dot(v) >= LIMIT &&	// dotCB
		mNormals[2].dot(v) >= LIMIT &&	// dotDC
		mNormals[3].dot(v) >= LIMIT;	// dotAD
}
