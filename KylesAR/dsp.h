/*
	So far only touch is using anything here (just LPF too)
*/

#pragma once

#include <vector>
#include <stdio.h>
#include <iostream>
#include <stddef.h>
#include <math.h>
#include <deque>
#include <chrono>

#include <glm.hpp>
#include <opencv2\core.hpp>

#include "kibblesmath.h"

//for complementary filter
#define ACCELEROMETER_SENSITIVITY 8192.f
#define GYROSCOPE_SENSITIVITY 65.536f

//epsilon for sg smooth
const double TINY_FLOAT = 1.0e-300;
//invalid time for 1 euro filter
const double UndefinedTime = -1.0;
//array of doubles for sg smooth
typedef std::vector<float> float_vec;
//array of ints for sg smooth
typedef std::vector<int> int_vec;

//complementary smoothing
void ComplementaryFilter(short accData[3], short gyrData[3], float *pitch, float *roll);

//savitzky golay smoothing
float_vec sg_coeff(const float_vec &b, const size_t deg);
float_vec sg_smooth(const float_vec &v, const int w, const int deg);

//low pass filter
class LowPassFilter {
	double y, a, s;
	bool initialized;

	void setAlpha(double alpha);
public:
	
	LowPassFilter(double alpha, double initval = 0.0);

	double filter(double value);
	double filterWithAlpha(double value, double alpha);

	bool hasLastRawValue(void) { return initialized; }
	double lastRawValue(void) { return y; }
};

//1 euro filter
class OneEuroFilter {
	LowPassFilter x, dx;
	double freq, mincutoff, beta_, dcutoff, lasttime;
	
	double alpha(double cutoff);

public:
	OneEuroFilter(double freq, double mincutoff = 1.0, double beta_ = 0.0, double dcutoff = 1.0);
	~OneEuroFilter(void);

	void setFrequency(double f);
	void setMinCutoff(double mc);
	void setBeta(double b);
	void setDerivateCutoff(double dc);

	double filter(double value, double timestamp = UndefinedTime);
};

//extended kalman filter
class ExtendedKalman {
public:
	cv::Mat P_pre, x_pre, x_post, P_post, A, F, H, Q, R, I, G;
	int n, m;

	//creates an object with N states, M observables, and specified prediction, process, and measurement noise.
	ExtendedKalman(int _n, int _m, float pval = 0.1f, float qval = 0.0001f, float rval = 0.1f);

	//new prediction
	cv::Mat predict();
	cv::Mat update(cv::Mat x);

	int getStates() { return n; }
	int getObservables() { return m; }
};

struct Measurement {
public:
	float weight, accuracy, value;
	Measurement(float w, float a, float v) {
		weight = w;
		accuracy = a;
		value = v;
	}
};

class RationalProbability {
public:
	std::vector<Measurement> measurements = std::vector<Measurement>();

	float pow2(float x) { return x * x; }
	// value at x
	float function1D(float x);
	// deriv at x
	float dFunction1D(float x);
	// deriv equals 0
	float solve(int iterations, float eps);
	// takes in measurement weight (default 1.0f), accuracy (default 1.0f)
	void addMeasurement(float weight, float accuracy, float value);
};

// KB:	This is like a super simple Savitzkey-Golay-like filter of order 2 with a simple cutoff
//		In the near future instead of a cutoff we can Hermite smooth the coefficients much better.
// TODO:
//		Apply rotation cutoff
//		Derivative smoothing
//		Matrix mul may be backwards :P

typedef std::chrono::high_resolution_clock hrClock;
typedef hrClock::time_point timePoint;

using namespace glm;

class sgFilter {
public:
	// 3 is necessary; a general solution would compute the (numDataPoints-1)th derivative; maybe later ;)
	int numPoints;
	// 1st and 2nd order cutoffs
	float vCutoff, aCutoff, VCutoff, ACutoff;
	// the actual state of location
	vec3 v, a;
	// the actual state of orientation
	mat3 V, A;
	// awe yeah
	std::deque<vec3> l;
	std::deque<mat3> R;
	std::deque<timePoint> times;

	// attempts to model motion up to a certian maximum
	sgFilter(float velocityCutoff, float accelerationCutoff, float angularVelCutoff, float angularAccCutoff);
	sgFilter();

	// calc 1st and 2nd derivatives for prediction
	void update(mat4 pose);

	// call this once per frame rendered
	mat4 predict(float future = 0.f);
};
