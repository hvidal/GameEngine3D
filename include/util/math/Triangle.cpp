#include "Triangle.h"
#include "Plane.h"


Triangle::Triangle(const btVector3 &point0, const btVector3 &point1, const btVector3 &point2) noexcept:
	mPoints{point0, point1, point2}
{
	// We need the normal vector of each segment in order to check if the point is inside the triangle
	// Since this is 3-dimensions, finding the normals isn't trivial
	// First we need to do a cross product and find the normal of the plane where the triangle is

	const btVector3&& v01 = mPoints[1] - mPoints[0];
	const btVector3&& v02 = mPoints[2] - mPoints[0];
	const btVector3&& v12 = mPoints[2] - mPoints[1];
	const btVector3&& planeNormal = v01.cross(v02);

	// The normal of each segment is perpendicular to (1) the planeNormal and (2) the segment vector

	mNormal01 = planeNormal.cross(v01);
	if (mNormal01.dot(v12) < 0.0f)
		mNormal01 = -mNormal01;

	mNormal02 = planeNormal.cross(v02);
	if (mNormal02.dot(v01) < 0.0f)
		mNormal02 = -mNormal02;

	mNormal12 = planeNormal.cross(v12);
	if (mNormal12.dot(-v01) < 0.0f)
		mNormal12 = -mNormal12;
}

static constexpr const float LIMIT = -0.001f;

bool Triangle::isInside(const btVector3 &point) const noexcept {
	return
		(point - mPoints[0]).dot(mNormal01) >= LIMIT &&
		(point - mPoints[1]).dot(mNormal12) >= LIMIT &&
		(point - mPoints[2]).dot(mNormal02) >= LIMIT;

}


bool Triangle::isInterceptedBy(const Line &line, btVector3 &interceptionPoint) const {
	const btVector3& linePoint = line.getPoint();
	const btVector3& lineDir1 = line.getDirectionUnit();

	const btVector3&& v0 = mPoints[0] - linePoint;
	const btVector3&& v1 = mPoints[1] - linePoint;
	const btVector3&& v2 = mPoints[2] - linePoint;

	// Find the normal vectors for each plane.
	// The dot product of the normal with another point should be positive
	// So we revert the normal if this is not the case
	btVector3 normal01 = v0.cross(v1);
	if (normal01.dot(v2) < 0.0f)
		normal01 = -normal01;

	btVector3 normal02 = v0.cross(v2);
	if (normal02.dot(v1) < 0.0f)
		normal02 = -normal02;

	btVector3 normal12 = v1.cross(v2);
	if (normal12.dot(v0) < 0.0f)
		normal12 = -normal12;

	// Project one of the triangle points on the line so that we can check if that
	// point is inside the planes
	const float projection = v0.dot(lineDir1);
	const btVector3&& point = linePoint + projection * lineDir1;
	const btVector3&& vp = point - linePoint;
	if (normal01.dot(vp) >= LIMIT && normal02.dot(vp) >= LIMIT && normal12.dot(vp) >= LIMIT) {
		interceptionPoint = Plane(*this).getInterception(line);
		return true;
	}
	return false;
}


