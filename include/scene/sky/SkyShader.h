#ifndef GAMEDEV3D_SKYSHADER_H
#define GAMEDEV3D_SKYSHADER_H

class SkyShader {
public:
	static constexpr const char* SHARED_FRAGMENT_CODE =
		"uniform vec3 cameraPosition;"
		"uniform vec3 sunPosition;"
		"uniform float planetRadius;"
		"uniform float dotOriginToSun;"
		"uniform float dotOriginToHorizon;"
		"uniform float dotOriginToWaterLevel;"

		"const float hazeDotLimit = 0.25;" // ~15 degrees (cosine)
		"const float dayLimit = 0.15;" // degrees with the horizon (cosine)
		"const float sunSize = 0.999;" // dot(vCameraToSun, vCameraToPoint)
		"const float sunGlow = 0.995;"

		"const vec3 DAY = vec3(73, 178, 252) / 255.0;"
		"const vec3 DAY_HORIZON = vec3(186, 220, 255) / 255.0;"
		"const vec3 NIGHT = vec3(0.1, 0.1, 0.1);"
		"const vec3 NIGHT_HORIZON = vec3(34, 42, 65) / 255.0;"
		"const vec3 SUN_COLOR = vec3(1.0, 0.99, 0.8);"
		"const vec3 SUNSET_COLOR = vec3(245, 135, 45) / 255.0;"
		"const vec3 SUNSET_HORIZON_COLOR = vec3(160, 89, 31) / 255.0;"

		"float sky_dotOriginToPoint(vec3 point) {"
			"vec3 vCameraToPoint1 = normalize(point - cameraPosition);"
			"vec3 vCameraToOrigin1 = normalize(-cameraPosition);"
			"return dot(vCameraToPoint1, vCameraToOrigin1);"
		"}"

		"vec4 sky_color(vec3 point, float dotOriginToPoint, bool renderSun) {"
			"float haze = 0.0;"
			"float diff = dotOriginToHorizon - dotOriginToPoint;"
			"if (diff < hazeDotLimit) {"
				"haze = (hazeDotLimit - diff) / hazeDotLimit;"
			"}"

			"float sunDiff = dotOriginToHorizon - dotOriginToSun;"

			"vec3 sky, horizon;"
			"if (sunDiff >= dayLimit) {" // DAY LIGHT
				"sky = DAY;"
				"horizon = DAY_HORIZON;"
			"} else if (sunDiff <= -dayLimit) {" // NIGHT
				"sky = NIGHT;"
				"horizon = NIGHT_HORIZON;"
			"} else {"
				// day_night --> night .. day --> 0.0 .. 1.0
				"float day_night = (sunDiff + dayLimit) / (2.0 * dayLimit);"
				"sky = mix(NIGHT, DAY, day_night);"
				"horizon = mix(NIGHT_HORIZON, DAY_HORIZON, day_night);"
			"}"

			// render the sun and its glow
			"if (sunDiff >= -dayLimit) {" // if sun is visible (including its glow)
				"vec3 vCameraToSun1 = normalize(sunPosition - cameraPosition);"
				"vec3 vCameraToPoint1 = normalize(point - cameraPosition);"
				"float dotSun = dot(vCameraToSun1, vCameraToPoint1);"

				// The light around the sun depends on the position of the sun.
				// If it is close to the horizon, it will shine more (atmospheric scattering).
				"if (sunDiff <= dayLimit) {" // if close to the horizon
					"const float minDotSun = 0.65;" // max glow size around the sun
					"if (dotSun >= minDotSun) {"
						// shineFactor = 0.0 .. 1.0
						// sunDiff ranges from dayLimit to -dayLimit
						// shineFactor is 1.0 when sunDiff is 0.0 (mid point)
						"float shineFactor = sunDiff >= 0.0? (1.0 - sunDiff / dayLimit) : (1.0 - abs(sunDiff / dayLimit));"
						// shineStart = cos(angle) where it should start the glow around the sun
						// if shineFactor is 0.0, then the sun has a standard shine (shineStart = sunGlow)
						// if shineFactor is 1.0, then the sun has maximum shine (shineStart = minDotSun)
						"float shineStart = sunSize - shineFactor * (sunSize - minDotSun);"
						"shineStart = min(shineStart, sunGlow);"

						"if (dotSun >= shineStart) {" // if this pixel is in the shining area
							// inc increases as it goes closer to the sun
							// inc is 0.0 on the shineStart, and 1.0 when dotSun = sunSize
							"float maxDiff = sunSize - shineStart;"
							"float diff = min(dotSun, sunSize) - shineStart;"
							"float inc = diff / maxDiff;"

							"if (renderSun) {"
								"vec3 shineColor = dotSun >= sunSize? SUN_COLOR : mix(SUN_COLOR, SUNSET_COLOR, shineFactor);"
								"sky = mix(sky, shineColor, inc * inc * inc);" // cubic interpolation
							"} else {"
								"sky = mix(sky, SUNSET_COLOR, inc * inc * inc);" // cubic interpolation
							"}"
						"}"
					"}"

					"if (haze > 0.0 && dotSun > 0.0) {"
						"if (sunDiff >= 0.0) {" // sun is setting
							// Horizon color ranges from day to sunset
							"float factor = 1.0 - sunDiff / dayLimit;"
							"horizon = mix(horizon, SUNSET_HORIZON_COLOR, factor * dotSun);"
						"} else {" // sun is set, still going down
							// Horizon color ranges from sunset to night
							"float factor = 1.0 - abs(sunDiff / dayLimit);"
							"horizon = mix(horizon, SUNSET_HORIZON_COLOR, factor * dotSun);"
						"}"
					"}"
				"} else if (dotSun >= sunGlow) {"
					// inc increases as it goes closer to the sun
					// sunGlow .. sunSize --> 0.0 .. 1.0
					"float inc = (dotSun - sunGlow) / (sunSize - sunGlow);"
					"sky = mix(sky, SUN_COLOR, inc * inc * inc);" // cubic interpolation
				"}"
			"}"

			// apply the haze, if close to the horizon
			"vec3 c = mix(sky, horizon, haze * haze * haze);" // cubic interpolation
			"return vec4(c, 1.0);" // cubic interpolation
		"}"

		"const float hazeStart = 200.0;"
		"const float hazeEnd = 3000.0;"
		"const float hazeDiff = hazeEnd - hazeStart;"

		"vec4 sky_mixHaze(vec4 color, float distance, vec3 point) {"
			"if (distance < hazeStart)"
				"return color;"
			"float dotOriginToPoint = sky_dotOriginToPoint(point);"
			"if (dotOriginToPoint > dotOriginToHorizon)"
				"dotOriginToPoint = dotOriginToHorizon;"

			"float factor = (min(distance, hazeEnd) - hazeStart) / hazeDiff;"
			"factor = 0.5 * factor;" // landscape should never be completely hidden
			"vec4 c = sky_color(point, dotOriginToPoint, false);"
			"return mix(color, c, factor);"
		"}"

		"uniform float ambientLight;"
		"uniform float diffuseLight;"

		"vec4 sky_lighting(vec4 material, vec3 eyeV, vec3 eyeN, vec3 eyeL, float shadow) {"
			"vec3 n = normalize(eyeN);"
			"vec3 s = normalize(eyeL - eyeV);"
			"float dotSN = dot(s, n);"
			"float shading = max(dotSN, 0.0);"
			"float occlusion = dotSN < 0.0? 1.0 - abs(dotSN) : 1.0;"

			"vec4 Ka = material * ambientLight * occlusion;"
			"vec4 Kd = material * diffuseLight * shading * shadow * occlusion;"
			"return Ka + Kd;"
		"}";
};


#endif //GAMEDEV3D_SKYSHADER_H
