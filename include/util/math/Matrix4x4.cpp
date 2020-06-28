#include "Matrix4x4.h"


Matrix4x4::Matrix4x4(const btTransform& transform) {
	*this = transform;
}


Matrix4x4 &Matrix4x4::operator=(const btTransform& transform) {
	const auto& basis = transform.getBasis();
	setRow(0, basis.getRow(0));
	setRow(1, basis.getRow(1));
	setRow(2, basis.getRow(2));
	setOrigin(transform.getOrigin());
	return *this;
}


void Matrix4x4::identity() noexcept {
	mData[0] = mData[5] = mData[10] = mData[15] = 1.0f;
	mData[1] = mData[2] = mData[3] = 0.f;
	mData[4] = mData[6] = mData[7] = 0.f;
	mData[8] = mData[9] = mData[11] = 0.f;
	mData[12] = mData[13] = mData[14] = 0.f;
}


void Matrix4x4::setRow(unsigned int index, const btVector3 &row) {
	unsigned int c = 4 * index;
	mData[c] = row.x();
	mData[c+1] = row.y();
	mData[c+2] = row.z();
	mData[c+3] = row.w();
}


void Matrix4x4::setOrigin(const btVector3 &origin) {
	mData[12] = origin.x();
	mData[13] = origin.y();
	mData[14] = origin.z();
	mData[15] = 1.0f;
}


Matrix4x4& Matrix4x4::translate(const btVector3& offset) noexcept {
	mData[12] = mData[0] * offset.x() + mData[4] * offset.y() + mData[8] * offset.z() + mData[12];
	mData[13] = mData[1] * offset.x() + mData[5] * offset.y() + mData[9] * offset.z() + mData[13];
	mData[14] = mData[2] * offset.x() + mData[6] * offset.y() + mData[10] * offset.z() + mData[14];
	mData[15] = mData[3] * offset.x() + mData[7] * offset.y() + mData[11] * offset.z() + mData[15];
	return *this;
}


Matrix4x4& Matrix4x4::multiplyRight(const Matrix4x4& right) noexcept {
	float nm00 = mData[0] * right.mData[0] + mData[4] * right.mData[1] + mData[8] * right.mData[2] + mData[12] * right.mData[3];
	float nm01 = mData[1] * right.mData[0] + mData[5] * right.mData[1] + mData[9] * right.mData[2] + mData[13] * right.mData[3];
	float nm02 = mData[2] * right.mData[0] + mData[6] * right.mData[1] + mData[10] * right.mData[2] + mData[14] * right.mData[3];
	float nm03 = mData[3] * right.mData[0] + mData[7] * right.mData[1] + mData[11] * right.mData[2] + mData[15] * right.mData[3];
	float nm10 = mData[0] * right.mData[4] + mData[4] * right.mData[5] + mData[8] * right.mData[6] + mData[12] * right.mData[7];
	float nm11 = mData[1] * right.mData[4] + mData[5] * right.mData[5] + mData[9] * right.mData[6] + mData[13] * right.mData[7];
	float nm12 = mData[2] * right.mData[4] + mData[6] * right.mData[5] + mData[10] * right.mData[6] + mData[14] * right.mData[7];
	float nm13 = mData[3] * right.mData[4] + mData[7] * right.mData[5] + mData[11] * right.mData[6] + mData[15] * right.mData[7];
	float nm20 = mData[0] * right.mData[8] + mData[4] * right.mData[9] + mData[8] * right.mData[10] + mData[12] * right.mData[11];
	float nm21 = mData[1] * right.mData[8] + mData[5] * right.mData[9] + mData[9] * right.mData[10] + mData[13] * right.mData[11];
	float nm22 = mData[2] * right.mData[8] + mData[6] * right.mData[9] + mData[10] * right.mData[10] + mData[14] * right.mData[11];
	float nm23 = mData[3] * right.mData[8] + mData[7] * right.mData[9] + mData[11] * right.mData[10] + mData[15] * right.mData[11];
	float nm30 = mData[0] * right.mData[12] + mData[4] * right.mData[13] + mData[8] * right.mData[14] + mData[12] * right.mData[15];
	float nm31 = mData[1] * right.mData[12] + mData[5] * right.mData[13] + mData[9] * right.mData[14] + mData[13] * right.mData[15];
	float nm32 = mData[2] * right.mData[12] + mData[6] * right.mData[13] + mData[10] * right.mData[14] + mData[14] * right.mData[15];
	float nm33 = mData[3] * right.mData[12] + mData[7] * right.mData[13] + mData[11] * right.mData[14] + mData[15] * right.mData[15];

	mData[0] = nm00; mData[1] = nm01; mData[2] = nm02; mData[3] = nm03;
	mData[4] = nm10; mData[5] = nm11; mData[6] = nm12; mData[7] = nm13;
	mData[8] = nm20; mData[9] = nm21; mData[10] = nm22; mData[11] = nm23;
	mData[12] = nm30; mData[13] = nm31; mData[14] = nm32; mData[15] = nm33;
	return *this;
}


Matrix4x4& Matrix4x4::multiplyLeft(const Matrix4x4& left) noexcept {
	float nm00 = left.mData[0] * mData[0] + left.mData[4] * mData[1] + left.mData[8] * mData[2] + left.mData[12] * mData[3];
	float nm01 = left.mData[1] * mData[0] + left.mData[5] * mData[1] + left.mData[9] * mData[2] + left.mData[13] * mData[3];
	float nm02 = left.mData[2] * mData[0] + left.mData[6] * mData[1] + left.mData[10] * mData[2] + left.mData[14] * mData[3];
	float nm03 = left.mData[3] * mData[0] + left.mData[7] * mData[1] + left.mData[11] * mData[2] + left.mData[15] * mData[3];
	float nm10 = left.mData[0] * mData[4] + left.mData[4] * mData[5] + left.mData[8] * mData[6] + left.mData[12] * mData[7];
	float nm11 = left.mData[1] * mData[4] + left.mData[5] * mData[5] + left.mData[9] * mData[6] + left.mData[13] * mData[7];
	float nm12 = left.mData[2] * mData[4] + left.mData[6] * mData[5] + left.mData[10] * mData[6] + left.mData[14] * mData[7];
	float nm13 = left.mData[3] * mData[4] + left.mData[7] * mData[5] + left.mData[11] * mData[6] + left.mData[15] * mData[7];
	float nm20 = left.mData[0] * mData[8] + left.mData[4] * mData[9] + left.mData[8] * mData[10] + left.mData[12] * mData[11];
	float nm21 = left.mData[1] * mData[8] + left.mData[5] * mData[9] + left.mData[9] * mData[10] + left.mData[13] * mData[11];
	float nm22 = left.mData[2] * mData[8] + left.mData[6] * mData[9] + left.mData[10] * mData[10] + left.mData[14] * mData[11];
	float nm23 = left.mData[3] * mData[8] + left.mData[7] * mData[9] + left.mData[11] * mData[10] + left.mData[15] * mData[11];
	float nm30 = left.mData[0] * mData[12] + left.mData[4] * mData[13] + left.mData[8] * mData[14] + left.mData[12] * mData[15];
	float nm31 = left.mData[1] * mData[12] + left.mData[5] * mData[13] + left.mData[9] * mData[14] + left.mData[13] * mData[15];
	float nm32 = left.mData[2] * mData[12] + left.mData[6] * mData[13] + left.mData[10] * mData[14] + left.mData[14] * mData[15];
	float nm33 = left.mData[3] * mData[12] + left.mData[7] * mData[13] + left.mData[11] * mData[14] + left.mData[15] * mData[15];

	mData[0] = nm00; mData[1] = nm01; mData[2] = nm02; mData[3] = nm03;
	mData[4] = nm10; mData[5] = nm11; mData[6] = nm12; mData[7] = nm13;
	mData[8] = nm20; mData[9] = nm21; mData[10] = nm22; mData[11] = nm23;
	mData[12] = nm30; mData[13] = nm31; mData[14] = nm32; mData[15] = nm33;
	return *this;
}


Matrix4x4& Matrix4x4::rotate(float phi, float x, float y, float z) noexcept {
	float sinAngle = sin(phi);
	float cosAngle = cos(phi);
	return rotate(sinAngle, cosAngle, x, y, z);
}


Matrix4x4& Matrix4x4::rotate(float sinAngle, float cosAngle, float x, float y, float z) noexcept {
	Matrix4x4 m;
	float _1_Minus_CosAngle = 1.0f - cosAngle;
	float x0 = x * _1_Minus_CosAngle;
	m.mData[0] = cosAngle + x * x0;
	m.mData[1] = z * sinAngle + y * x0;
	m.mData[2] = -y * sinAngle + z * x0;
	m.mData[3] = 0.f;

	float y0 = y * _1_Minus_CosAngle;
	m.mData[4] = -z * sinAngle + x * y0;
	m.mData[5] = cosAngle + y * y0;
	m.mData[6] = x * sinAngle + z * y0;
	m.mData[7] = 0.f;

	float z0 = z * _1_Minus_CosAngle;
	m.mData[8] = y * sinAngle + x * z0;
	m.mData[9] = -x * sinAngle + y * z0;
	m.mData[10] = cosAngle + z * z0;
	m.mData[11] = 0.f;

	m.mData[12] = 0.f;
	m.mData[13] = 0.f;
	m.mData[14] = 0.f;
	m.mData[15] = 1.f;

	multiplyRight(m);
	return *this;
}

Matrix4x4& Matrix4x4::lookAt(const btVector3& eye, const btVector3& center, const btVector3& up) noexcept {
	const btVector3& zAxis = -(center - eye).normalized();
	const btVector3& xAxis = up.cross(zAxis).normalized();
	const btVector3& yAxis = zAxis.cross(xAxis);

	mData[0] = xAxis.x(); mData[1] = yAxis.x(); mData[2] = zAxis.x(); mData[3] = 0.f;
	mData[4] = xAxis.y(); mData[5] = yAxis.y(); mData[6] = zAxis.y(); mData[7] = 0.f;
	mData[8] = xAxis.z(); mData[9] = yAxis.z(); mData[10] = zAxis.z(); mData[11] = 0.f;
	mData[12] = xAxis.dot(-eye); mData[13] = yAxis.dot(-eye); mData[14] = zAxis.dot(-eye); mData[15] = 1.f;

	return *this;
}


Matrix4x4& Matrix4x4::perspective(float fovy, float aspect, float zNear, float zFar) noexcept {
	static constexpr const float PI_OVER_360 = 0.0087266462599716478846184538424431f;
	float h = 1.0f / tan(fovy * PI_OVER_360); // cotangent
	float zNegDepth = zNear - zFar;
	mData[0] = h / aspect; mData[1] = 0.0f; mData[2] = 0.0f; mData[3] = 0.0f;
	mData[4] = 0.0f; mData[5] = h; mData[6] = 0.0f; mData[7] = 0.0f;
	mData[8] = 0.0f; mData[9] = 0.0f; mData[10] = (zFar + zNear) / zNegDepth; mData[11] = -1.0f;
	mData[12] = 0.0f; mData[13] = 0.0f; mData[14] = 2.0f * zFar * zNear / zNegDepth; mData[15] = 0.0f;
	return *this;
}


Matrix4x4& Matrix4x4::ortho(float left, float right, float bottom, float top, float near, float far) noexcept {
	float rl = right - left;
	float tb = top - bottom;
	float fn = far - near;
	assert(rl != 0);
	assert(tb != 0);
	assert(fn != 0);

	float _12 = -(right + left) / rl;
	float _13 = -(top + bottom) / tb;
	float _14 = -(far + near) / fn;
	mData[0] = 2.0f / rl; mData[1] = 0.0f; mData[2] = 0.0f; mData[3] = 0.0f;
	mData[4] = 0.0f; mData[5] = 2.0f / tb; mData[6] = 0.0f; mData[7] = 0.0f;
	mData[8] = 0.0f; mData[9] = 0.0f; mData[10] = -2.0f / fn; mData[11] = 0.0f;
	mData[12] = _12; mData[13] = _13; mData[14] = _14; mData[15] = 1.0f;
	return *this;
}


void Matrix4x4::debug() const {
	Log::info("4x4[%.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f | %.2f %.2f %.2f %.2f]", mData[0], mData[1], mData[2], mData[3], mData[4], mData[5], mData[6], mData[7], mData[8], mData[9], mData[10], mData[11], mData[12], mData[13], mData[14], mData[15]);
}




