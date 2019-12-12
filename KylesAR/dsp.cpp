#include "dsp.h"

//a fine-tuned 6 dof IMU filter for embedded systems
void ComplementaryFilter(short accData[3], short gyrData[3], float *pitch, float *roll) {
	float pitchAcc, rollAcc;

	// Integrate the gyroscope data -> int(angularSpeed) = angle
	*pitch += ((float)gyrData[0] / GYROSCOPE_SENSITIVITY) * .01; // Angle around the X-axis
	*roll -= ((float)gyrData[1] / GYROSCOPE_SENSITIVITY) * .01;    // Angle around the Y-axis

	// Compensate for drift with accelerometer data if !bullshit
	// Sensitivity = -2 to 2 G at 16Bit -> 2G = 32768 && 0.5G = 8192
	int forceMagnitudeApprox = abs(accData[0]) + abs(accData[1]) + abs(accData[2]);
	if (forceMagnitudeApprox > 8192 && forceMagnitudeApprox < 32768) {
		// Turning around the X axis results in a vector on the Y-axis
		pitchAcc = atan2f((float)accData[1], (float)accData[2]) * 180.f / M_PI;
		*pitch = *pitch * .98f + pitchAcc * .02f;

		// Turning around the Y axis results in a vector on the X-axis
		rollAcc = atan2f((float)accData[0], (float)accData[2]) * 180.f / M_PI;
		*roll = *roll * .98f + rollAcc * .02f;
	}
}


//calculates coefficients for sg smooth
float_vec sg_coeff(const float_vec &b, const int deg) {
	const int rows = int(b.size()), cols = deg + 1;
	cv::Mat A(rows, cols, CV_32FC1);
	float_vec res(rows);

	// generate input matrix for least squares fit
	int i, j;
	for (i = 0; i < rows; ++i) {
		for (j = 0; j < cols; ++j) {
			A.at<float>(i, j) = powf(float(i), float(j));
		}
	}

	cv::Mat tA, itA, tab, c;
	cv::transpose(A, tA);
	cv::invert(tA * A, itA);
	tab = (tA * cv::Mat(b));
	c = itA * tab;

	for (i = 0; i < b.size(); ++i) {
		res[i] = c.at<float>(0, 0);
		for (j = 1; j <= deg; ++j) {
			res[i] += c.at<float>(j, 0) * powf(float(i), float(j));
		}
	}

	return res;
}
//savitzky golay smoothing
float_vec sg_smooth(const float_vec &v, const int width, const int deg) {
	float_vec res(v.size(), 0.0);
	if ((width < 1) || (deg < 1) || (v.size() < (2 * width + 2))) {
		printf("sgsmooth: parameter error.\n");
		return res;
	}

	const int window = 2 * width + 1;
	const int endidx = int(v.size()) - 1;

	// handle border cases first because we need different coefficients
	int i, j;
	for (i = 0; i < width; ++i) {
		float_vec b1(window, 0.0);
		b1[i] = 1.0;
		float_vec c1 = sg_coeff(b1, deg);
		
		for (j = 0; j < window; ++j) {
			res[i] += c1[j] * v[j];
			res[endidx - i] += c1[j] * v[endidx - j];
		}
	}
	
	// now loop over rest of data. reusing the "symmetric" coefficients.
	float_vec b2(window, 0.0);
	b2[width] = 1.0;
	const float_vec c2(sg_coeff(b2, deg));
	
	for (i = 0; i <= (v.size() - window); ++i) {
		for (j = 0; j < window; ++j) {
			res[i + width] += c2[j] * v[i + j];
		}
	}

	return res;
}


//simple lpf
LowPassFilter::LowPassFilter(double alpha, double initval) {
	y = s = initval;
	setAlpha(alpha);
	initialized = false;
}
void LowPassFilter::setAlpha(double alpha) {
	if (alpha <= 0.0 || alpha > 1.0)
		throw std::range_error("alpha should be in (0.0., 1.0]");
	a = alpha;
}
double LowPassFilter::filter(double value) {
	double result;
	if (!initialized) {
		result = value;
		initialized = true;
	}
	else {
		result = a*value + (1.0 - a)*s;
	}
	y = value;
	s = result;
	return result;
}
double LowPassFilter::filterWithAlpha(double value, double alpha) {
	setAlpha(alpha);
	return filter(value);
}


//simple one euro filter
OneEuroFilter::OneEuroFilter(double freq, double mincutoff, double beta_, double dcutoff) : x(alpha(mincutoff)), dx(alpha(dcutoff)) {
	setFrequency(freq);
	setMinCutoff(mincutoff);
	setBeta(beta_);
	setDerivateCutoff(dcutoff);
	lasttime = UndefinedTime;
}
double OneEuroFilter::alpha(double cutoff) {
	double te = 1.0 / freq;
	double tau = 1.0 / (2.0 * M_PI * cutoff);
	return 1.0 / (1.0 + tau / te);
}
void OneEuroFilter::setFrequency(double f) {
	if (f <= 0.) throw std::range_error("freq should be >0");
	freq = f;
}
void OneEuroFilter::setMinCutoff(double mc) {
	if (mc <= 0) throw std::range_error("mincutoff should be >0");
	mincutoff = mc;
}
void OneEuroFilter::setBeta(double b) {
	beta_ = b;
}
void OneEuroFilter::setDerivateCutoff(double dc) {
	if (dc <= 0) throw std::range_error("dcutoff should be >0");
	dcutoff = dc;
}
double OneEuroFilter::filter(double value, double timestamp) {
	// update the sampling frequency based on timestamps
	if (lasttime != UndefinedTime && timestamp != UndefinedTime) {
		double delta = (timestamp - lasttime);
		freq = 1.0 / (delta == 0.0 ? 0.0001 : delta);
	}

	lasttime = timestamp;
	// estimate the current variation per second
	double dvalue = x.hasLastRawValue() ? (value - x.lastRawValue())*freq : 0.0; // FIXME: 0.0 or value?
	
	double edvalue = dx.filterWithAlpha(dvalue, alpha(dcutoff));
	// use it to update the cutoff frequency
	double cutoff = mincutoff + beta_*fabs(edvalue);

	// filter the given value
	return x.filterWithAlpha(value, alpha(cutoff));

}
OneEuroFilter::~OneEuroFilter(void) {}


//creates an object with N states, M observables, and specified prediction, process, and measurement noise.
ExtendedKalman::ExtendedKalman(int _n, int _m, float pval, float qval, float rval) {
	n = _n;
	m = _m;
	//currrent state is zero
	x_pre = cv::Mat::zeros(cv::Size(n, 1), CV_32FC1);
	x_post = cv::Mat::zeros(cv::Size(n, 1), CV_32FC1);
	//identity noise covariance matrix
	P_post = cv::Mat::eye(cv::Size(n, n), CV_32FC1) * pval;
	//get state and measurement jacobians
	F = x_pre = cv::Mat::zeros(cv::Size(n, m), CV_32FC1);
	H = x_pre = cv::Mat::zeros(cv::Size(n, m), CV_32FC1);
	//setup covariance matrix for process and measurement noise
	Q = cv::Mat::eye(cv::Size(n, n), CV_32FC1) * qval;
	R = cv::Mat::eye(cv::Size(m, m), CV_32FC1) * rval;
	//another identity
	I = cv::Mat::eye(cv::Size(n, n), CV_32FC1);
}
//new prediction
cv::Mat ExtendedKalman::predict() {
	x_pre = A * x_post;
	P_pre = A * P_post * A.t() + Q;
	return x_pre;
}
//new correction
cv::Mat ExtendedKalman::update(cv::Mat x) {
	G = P_pre * H.t() * (H * P_pre * H.t() + R);
	P_post = (I - G * H) * P_pre;
	x_post = x_pre + G * (x - x_pre);
	return x;
}


// value at x
float RationalProbability::function1D(float x) {
	if (measurements.size() == 0) return 0.0f;
	float a = 0.0f, b = 0.0f, c = 1.0f, d = 0.0f;
	for (int i = 0; i < measurements.size(); i++) {
		// m.weight (a,b), m.accuracy (j,k), m.value (c,d)
		Measurement m = measurements[i];
		// some caching
		float sqrtAcc = glm::sqrt(m.accuracy);
		// partials
		a += m.weight / pow2(m.accuracy * x - m.value) + 1.0f;
		b += sqrtAcc;
		c *= sqrtAcc;
		d += m.weight;
	}
	return measurements.size() * c * a / (M_PI * b * d);
}
// deriv at x
float RationalProbability::dFunction1D(float x) {
	if (measurements.size() == 0) return 0.0f;
	float a = 0.0f, b = 0.0f, c = 1.0f, d = 0.0f;
	for (int i = 0; i < measurements.size(); i++) {
		// m.weight (a,b), m.accuracy (j,k), m.value (c,d)
		Measurement m = measurements[i];
		// some caching
		float sqrtAcc = glm::sqrt(m.accuracy),
			axv = m.accuracy * x - m.value;
		// partials
		a -= (2.0f * m.weight * m.accuracy * axv) / pow2(axv*axv + 1.0f);
		b += sqrtAcc;
		c *= sqrtAcc;
		d += m.weight;
	}
	return measurements.size() * c * a / (M_PI * b * d);
}
// deriv equals 0
float RationalProbability::solve(int iterations, float eps) {
	if (measurements.size() == 0) return 0.0f;
	float guess = 0.0f;
	float a = 1.0f, b = 0.0f;
	// this guess has a high chance of being exact if accuracy (j,k) terms fit the data well
	for (int i = 0; i < measurements.size(); i++) {
		// m.weight (a,b), m.accuracy (j,k), m.value (c,d)
		Measurement m = measurements[i];
		float sqrtAcc = glm::sqrt(m.accuracy);
		a *= sqrtAcc;
		b += m.weight;
		guess += m.weight * m.value;
	}
	guess /= a * b;
	// gradient descent to refine just in case
	for (int i = 0; i < iterations; i++) {
		float dvalue = dFunction1D(guess);
		if (glm::abs(dvalue) < eps) break;
		guess += dvalue * eps;
	}
	return guess;
}
// takes in measurement weight (default 1.0f), accuracy (default 1.0f)
void RationalProbability::addMeasurement(float weight, float accuracy, float value) {
	measurements.push_back(Measurement(weight, accuracy, value));
}


sgFilter::sgFilter() {
	sgFilter(999., 999., 999., 999.);
}
// attempts to model motion up to a certian maximum
sgFilter::sgFilter(float velocityCutoff, float accelerationCutoff, float angularVelCutoff, float angularAccCutoff) {
	numPoints = 3;

	vCutoff = velocityCutoff;
	aCutoff = accelerationCutoff;
	VCutoff = angularVelCutoff;
	ACutoff = angularAccCutoff;

	v = vec3(0.);
	a = vec3(0.);
	V = mat3(1.);
	A = mat3(1.);

	l = std::deque<vec3>(numPoints);
	R = std::deque<mat3>(numPoints);
	times = std::deque<timePoint>(numPoints);
}
// calc 1st and 2nd derivatives for prediction
void sgFilter::update(mat4 pose) {

	R.push_front(mat3(pose));
	l.push_front(vec3(pose[3]));
	times.push_front(hrClock::now());

	// number of data points determines how many derivative we can do
	int count = l.size();

	// if we have zero or one point we can't do anything
	if (count < 2) {
		v = vec3(0.);
		a = vec3(0.);
		V = mat3(1.);
		A = mat3(1.);
		return;
	}

	// the velocity now
	v = l[0] - l[1];
	V = inverse(R[1]) * R[0];

	//apply the cutoff
	float mv = length(v);
	if (mv > vCutoff) v *= vCutoff / mv;
	//float mV = 1. - .1666666 * (3. + dot(vec3(1., 0., 0.), V[0]) + dot(vec3(0., 1., 0.), V[1]) + dot(vec3(0., 0., 1.), V[2]));
	//if (mV > VCutoff) 0 is no rotation, 1 is a complete flip

	// at three points we can do acc
	if (count > 2) {
		// the velocity from 1 measurement ago
		vec3 lv = l[1] - l[2];
		mat3 lV = inverse(R[2]) * R[1];
		a = v - lv;
		A = inverse(lV) * V;

		//apply the cutoff
		float ma = length(a);
		if (ma > aCutoff) a *= aCutoff / ma;
		//float mA = 1. - .1666666 * (3. + dot(vec3(1., 0., 0.), A[0]) + dot(vec3(0., 1., 0.), A[1]) + dot(vec3(0., 0., 1.), A[2]));
		//if (mA > ACutoff) 0 is no rotation, 1 is a complete flip

		if (count > 3) {
			//pop the back or something
			l.pop_back();
			R.pop_back();
			times.pop_back();
		}

	} else {
		// the acceleration cannot be computed
		a = vec3(0.);
		A = mat3(1.);

	}
}
// call this once per frame rendered
mat4 sgFilter::predict(float future) {
	int count = times.size();
	vec3 oL = vec3(0.);
	mat3 oR = mat3(1.);
	// we have nothing!
	if (count == 0) {
		return mat4(1.);

		// only position
	} else if (count < 2) {
		oL = l[0];
		oR = R[0];

		// position and velocity
	} else {
		// change in time from last observation
		float tSpan = (times[0] - times[1]).count(),
			tDelta = (future + (hrClock::now() - times[0]).count()) / tSpan;

		vec3 _v = vec3(v);
		mat3 _V = mat3(V);

		// add acceleration onto velocity
		if (count > 2) {
			// check if slerp needs to be from time[0] to time[2]
			_V = slerp(V, V * A, tDelta);
			_v = v + a * tDelta;
		}

		// add velocity onto location
		oR = slerp(R[0], R[0] * _V, tDelta);
		oL = l[0] + _v * tDelta;
	}

	return mat4(vec4(oR[0], 0.), vec4(oR[1], 0.), vec4(oR[2], 0.), vec4(oL, 1.));
}
