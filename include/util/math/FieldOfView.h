#ifndef FOURPLANES_H
#define FOURPLANES_H

#include <LinearMath/btVector3.h>


class FieldOfView {
	btVector3 mCenter;
	btVector3 mNormals[4];

public:
	FieldOfView(const btVector3& center, const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& d) noexcept;

	bool isVisible(const btVector3& point) const noexcept;
};


#endif
