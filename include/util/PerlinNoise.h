#pragma once

#include <math.h>

// http://www.dreamincode.net/forums/topic/66480-perlin-noise/
class PerlinNoise
{
	static double findnoise2(double x,double y)
	{
		int n = (int)x + (int)y * 57;
		n = (n<<13) ^ n;
		int nn = (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
		return 1.0 - ((double)nn / 1073741824.0);
	}

	static double interpolate(double a,double b,double x)
	{
		double ft = x * 3.1415927;
		double f = (1.0 - cos(ft)) * 0.5;
		return a * (1.0 - f) + b * f;
	}

	static int fastfloor(double x) {
		return x > 0? (int) x : (int) x-1;
	}

public:
	static double noise(double x,double y)
	{
		double floorx = fastfloor(x);
		double floory = fastfloor(y);
		double s = findnoise2(floorx, floory);
		double t = findnoise2(floorx+1, floory);
		double u = findnoise2(floorx, floory+1);
		double v = findnoise2(floorx+1, floory+1);
		double int1 = interpolate(s, t, x-floorx);
		double int2 = interpolate(u, v, x-floorx);
		return interpolate(int1, int2, y-floory);
	}
};
