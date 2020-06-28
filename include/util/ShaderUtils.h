#ifndef GAMEDEV3D_SHADERUTILS_H
#define GAMEDEV3D_SHADERUTILS_H


class ShaderUtils {
public:
	// http://www.neilmendoza.com/glsl-rotation-about-an-arbitrary-axis/
	static constexpr const char* ROTATION_MATRIX =
		"mat3 rotationMatrix(vec3 axis, float cosAngle) {"
			"axis = normalize(axis);"
			"float s = sqrt(1.0 - cosAngle * cosAngle);"
			"float c = cosAngle;"
			"float oc = 1.0 - c;"

			"return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,"
						"oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,"
						"oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);"
		"}";

	// http://stackoverflow.com/questions/6301085/how-to-check-if-an-object-lies-outside-the-clipping-volume-in-opengl
	static constexpr const char* IN_FRUSTUM =
		"bool inFrustum(vec4 p) {"
			"return abs(p.x) < p.w && "
				"abs(p.y) < p.w && "
				"0.0 < p.z && p.z < p.w;"
		"}";


	// Removes the translation components of a mat4
	static constexpr const char* TO_MAT3 =
		"mat3 toMat3(mat4 m) {"
			"return mat3(m[0].xyz, m[1].xyz, m[2].xyz);"
		"}";


	static constexpr const char* TO_PLANET_SURFACE =
		"const vec3 _y = vec3(0.0, 1.0, 0.0);"
		"vec3 toPlanetSurface(vec3 point, vec3 v) {"
			"vec3 _up = normalize(point);"
			"vec3 _left = cross(_y, _up);"
			"vec3 _front = cross(_up, _left);"
			"return mat3(_front, _up, _left) * v;"
		"}";
};

#endif

