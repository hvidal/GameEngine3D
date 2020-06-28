#include "Plane.h"


Plane::Plane(const btVector3 &point1, const btVector3 &point2, const btVector3 &point3) noexcept:
	mPoints{point1, point2, point3},
	mNormal((point2 - point1).cross(point3 - point1))
{}


Plane::Plane(const Triangle &triangle) noexcept:
	Plane(triangle.mPoints[0], triangle.mPoints[1], triangle.mPoints[2])
{}


btVector3 Plane::getInterception(const Line &line) const {
	// Line: P = p0 + vt
	// Plane: n . (P - q0) = 0
	// n . (p0 + vt - q0) = 0
	// n . (p0 - q0) + n . vt = 0
	// t = - n . (p0 - q0) / n . v

	const btVector3& p0 = line.getPoint();
	const btVector3& v1 = line.getDirectionUnit();

	float t = - mNormal.dot(p0 - mPoints[0]) / mNormal.dot(v1);
	return p0 + t * v1;
}



