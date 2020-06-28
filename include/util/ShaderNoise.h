#ifndef INCLUDE_UTIL_SHADERNOISE_H_
#define INCLUDE_UTIL_SHADERNOISE_H_

#include <SDL2/SDL_timer.h>
#include "../app/Interfaces.h"

class ShaderNoise {

	static float getTime() noexcept {
		return SDL_GetTicks() / 1500.f;
	}

public:
	static constexpr const char* SHADER_CODE =
		"uniform float time;"

		//	https://github.com/ashima/webgl-noise
		//	Copyright (C) 2011 Ashima Arts. All rights reserved.
		//	Distributed under the MIT License. See LICENSE file.
		//	https://github.com/ashima/webgl-noise
		//	SIMPLEX START
		"vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }"
		"vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }"
		"vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }"

		"float snoise(vec2 v) {"
			"const vec4 C = vec4(0.211324865405187,"  // (3.0-sqrt(3.0))/6.0
							  "0.366025403784439,"  // 0.5*(sqrt(3.0)-1.0)
							 "-0.577350269189626,"  // -1.0 + 2.0 * C.x
							  "0.024390243902439);" // 1.0 / 41.0
			"vec2 i = floor(v + dot(v, C.yy));"
			"vec2 x0 = v - i + dot(i, C.xx);"

			"vec2 i1;"
			"i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);"
			"vec4 x12 = x0.xyxy + C.xxzz;"
			"x12.xy -= i1;"

			"i = mod289(i);" // Avoid truncation effects in permutation
			"vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 )) + i.x + vec3(0.0, i1.x, 1.0 ));"
			"vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);"
			"m = m*m;"
			"m = m*m;"

			"vec3 x = 2.0 * fract(p * C.www) - 1.0;"
			"vec3 h = abs(x) - 0.5;"
			"vec3 ox = floor(x + 0.5);"
			"vec3 a0 = x - ox;"

			"m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );"

			"vec3 g;"
			"g.x  = a0.x  * x0.x  + h.x  * x0.y;"
			"g.yz = a0.yz * x12.xz + h.yz * x12.yw;"
			"return 130.0 * dot(m, g);"
		"}"
		// SIMPLEX END

		"const float GRADIENT_DELTA = 0.015;"
		"const float waveHeight1 = 0.03;"
		"const float waveHeight2 = 0.02;"
		"const float waveHeight3 = 0.001;"

		"float noiseAt(vec3 p) {"
			"return p.y + (0.5 + waveHeight1 + waveHeight2 + waveHeight3)"
				"+ snoise(vec2(p.x + time * 0.4, p.z + time * 0.5)) * waveHeight1"
				"+ snoise(vec2(p.x * 1.6 - time * 0.4, p.z * 1.7 - time * 0.6)) * waveHeight2"
				"+ snoise(vec2(p.x * 6.6 - time * 1.0, p.z * 2.7 + time * 1.176)) * waveHeight3;"
		"}"

		"vec3 noiseNormal(vec3 p) {"
			"float map_p = noiseAt(p);"
			"return normalize(vec3("
				"map_p - noiseAt(p - vec3(GRADIENT_DELTA, 0, 0)),"
				"map_p - noiseAt(p - vec3(0, GRADIENT_DELTA, 0)),"
				"map_p - noiseAt(p - vec3(0, 0, GRADIENT_DELTA))));"
		"}";

	static void setVars(IShader* shader) {
		shader->set("time", getTime());
	}

};


#endif
