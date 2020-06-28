#ifndef LINE_H
#define LINE_H

#include <LinearMath/btVector3.h>


class Line {
	const btVector3 mPoint;
	const btVector3 mDirection1;

public:
	Line(const btVector3& point, const btVector3& direction) noexcept:
		mPoint(point),
		mDirection1(direction.normalized())
	{}

	const btVector3& getPoint() const noexcept;
	const btVector3& getDirectionUnit() const noexcept;
	btVector3 closestPointTo(const btVector3& point) const noexcept;
	float distanceTo(const btVector3& point) const noexcept;
};

//-----------------------------------------------------------------------------

inline const btVector3& Line::getPoint() const noexcept
{ return mPoint; }

inline const btVector3& Line::getDirectionUnit() const noexcept
{ return mDirection1; }

inline float Line::distanceTo(const btVector3 &point) const noexcept
{ return closestPointTo(point).distance(point); }

inline btVector3 Line::closestPointTo(const btVector3& point) const noexcept {
	const btVector3& u = point - mPoint;
	float t = u.dot(mDirection1) / mDirection1.dot(mDirection1);
	return mPoint + mDirection1 * t;
}

#endif
