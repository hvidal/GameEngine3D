#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <LinearMath/btVector3.h>
#include "Line.h"


class Triangle {
	btVector3 mNormal01;
	btVector3 mNormal02;
	btVector3 mNormal12;

public:
	const btVector3 mPoints[3];

	Triangle(const btVector3& point1, const btVector3& point2, const btVector3& point3) noexcept;

	bool isInside(const btVector3& point) const noexcept;
	bool isInterceptedBy(const Line& line, btVector3& interceptionPoint) const;
};

//-----------------------------------------------------------------------------

#endif
