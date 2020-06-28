#ifndef PLANE_H
#define PLANE_H

#include <LinearMath/btVector3.h>
#include "Line.h"
#include "Triangle.h"


class Plane {
	const btVector3 mPoints[3];
	const btVector3 mNormal;

public:
	Plane(const btVector3& point1, const btVector3& point2, const btVector3& point3) noexcept;
	Plane(const Triangle& triangle) noexcept;

	btVector3 getInterception(const Line& line) const;
};

//-----------------------------------------------------------------------------


#endif
