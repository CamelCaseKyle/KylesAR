/*

*/

#pragma once

#define GLM_SWIZZLE

#include <opencv2\core.hpp>
#include <glm.hpp>
#include "rectExt.h"

#define M_PI2	6.2831853071794f
#define M_PI	3.1415926535897f
#define M_HPI	1.5707963267948f
#define INV255	0.0039215686274f
#define INV2PI	0.1591549430918f
#define DEG2RAD	0.0174532925199f
#define RAD2DEG	57.295779513082f
#define M_PIL	3.141592653589793238462643383279502884L
#define INV255L	0.003921568627450980392156862745098039L
#define INV2PIL	0.159154943091895335768883763372514362L

///////////////////////////////////////////////////////////// Scalar /////////////////////////////////////////////////////////////

//approximation
inline float _cos(float x) {
	x *= INV2PI;
	x -= 0.25f + std::floor(x + 0.25f);
	x *= 16.f * (std::abs(x) - 0.5f);
	return x;
}
//approximation
inline float _sin(float x) {
	x += M_HPI;
	x *= INV2PI;
	x -= 0.25f + std::floor(x + 0.25f);
	x *= 16.f * (std::abs(x) - 0.5f);
	return x;
}

//align 16 to use rsqrtss
inline void SSEsqrt(float * __restrict pOut, float * __restrict pIn) {
	__m128 in = _mm_load_ss(pIn);
	_mm_store_ss(pOut, _mm_mul_ss(in, _mm_rsqrt_ss(in)));
}
//vectorized rsqrtps
inline void SSElength(float *x, float *y, float *z, float *l, unsigned int length) {
	__m128 xmm0, xmm1, xmm2, xmm3;
	for (unsigned int i = 0; i < length; ++i) {
		xmm0 = _mm_load_ps(&x[i]);
		xmm1 = _mm_load_ps(&y[i]);
		xmm2 = _mm_load_ps(&z[i]);
		xmm3 = _mm_add_ps(_mm_mul_ps(xmm0, xmm0), _mm_mul_ps(xmm1, xmm1));
		xmm3 = _mm_add_ps(_mm_mul_ps(xmm2, xmm2), xmm3);
		xmm3 = _mm_sqrt_ps(xmm3);
		_mm_store_ps(&l[i], xmm3);
	}
}

///////////////////////////////////////////////////////////// Vector /////////////////////////////////////////////////////////////

//multiplication opencv
template<typename T> inline cv::Point_<T> operator*(const cv::Point_<T> &l, const cv::Point_<T> &r) { return cv::Point_<T>(l.x*r.x, l.y*r.y); }
//distance function
template<typename T> inline float operator|(const cv::Point_<T> &l, const cv::Point_<T> &r) { T dx = l.x - r.x, dy = l.y - r.y; return float(std::sqrt(dx*dx + dy*dy)); }
//dot operator
template<typename T> inline float operator%(const cv::Point_<T> &l, const cv::Point_<T> &r) { return float(l.x*r.x + l.y*r.y); }

//this lets us sort things
struct lt_cv_point_x {
	inline bool operator() (const cv::Point2f &pt1, const cv::Point2f &pt2) {
		return (pt1.x < pt2.x);
	}
};
struct lt_cv_point_y {
	inline bool operator() (const cv::Point2f &pt1, const cv::Point2f &pt2) {
		return (pt1.y < pt2.y);
	}
};

//convert
inline glm::vec3 mat2euler(glm::mat3 m) {
	// Assuming the angles are in radians.
	if (m[0][1] > 0.998) { // singularity at north pole
		return glm::vec3(atan2(m[2][0], m[2][2]), M_HPI, 0.0);
	}
	if (m[0][1] < -0.998) { // singularity at south pole
		return glm::vec3(atan2(m[2][0], m[2][2]), -M_HPI, 0.0);
	}
	return glm::vec3(atan2(-m[0][2], m[0][0]), asin(m[0][1]), atan2(-m[2][1], m[1][1]));
} //yxz?
inline glm::vec3 mat2euler(cv::Mat m) {
	// Assuming the angles are in radians.
	if (m.at<double>(1, 0) > 0.998) { // singularity at north pole
		return glm::vec3(atan2(m.at<double>(0, 2), m.at<double>(2, 2)), M_HPI, 0.0);
	}
	if (m.at<double>(1, 0) < -0.998) { // singularity at south pole
		return glm::vec3(atan2(m.at<double>(0, 2), m.at<double>(2, 2)), -M_HPI, 0.0);
	}
	return glm::vec3(atan2(-m.at<double>(2, 0), m.at<double>(0, 0)), asin(m.at<double>(1, 0)), atan2(-m.at<double>(1, 2), m.at<double>(1, 1)));
} //yxz?

//mine from shadertoy
inline glm::vec3 slerp(glm::vec3 start, glm::vec3 end, float percent) {
	float dse = glm::dot(start, end), theta = acos(dse)*percent;
	return start*cos(theta) + glm::normalize(end - start*dse)*sin(theta);
}
inline glm::mat3 slerp(glm::mat3 start, glm::mat3 end, float percent) {
	return glm::mat3(
		slerp(start[0], end[0], percent),
		slerp(start[1], end[1], percent),
		slerp(start[2], end[2], percent)
	);
}
inline glm::mat4 slerp(glm::mat4 start, glm::mat4 end, float percent) {
	glm::mat4 outp = glm::mat4(1.);
	outp[0] = glm::vec4(slerp(glm::vec3(start[0]), glm::vec3(end[0]), percent), 0.f);
	outp[1] = glm::vec4(slerp(glm::vec3(start[1]), glm::vec3(end[1]), percent), 0.f);
	outp[2] = glm::vec4(slerp(glm::vec3(start[2]), glm::vec3(end[2]), percent), 0.f);
	outp[3] = glm::vec4(glm::mix(glm::vec3(start[3]), glm::vec3(end[3]), percent), 1.f);
	return outp;
}
inline void basis(glm::vec3 &n, glm::vec3 &f, glm::vec3 &r) {
	float a = 1.f / (1.f + n.z);
	float b = -n.x*n.y*a;
	f = glm::vec3(1.f - n.x*n.x*a, b, -n.x);
	r = glm::vec3(b, 1.f - n.y*n.y*a, -n.y);
}
//unsigned distance from line segment
inline float usd_segment(cv::Point v, cv::Point w, cv::Point p) {
	cv::Point seg = v - w, iseg = w - v;
	//find projection of point p onto the line. 
	float t = std::min(0.f, std::max(1.f, ((p - v) % iseg) / (seg % seg)));
	// Projection falls on the segment
	return p | (v + t * iseg);
}
//2D line segments intersect
inline bool intersect(glm::vec2 &p0, glm::vec2 &p1, glm::vec2 &p2, glm::vec2 &p3, glm::vec2 *i) {
	glm::vec2 s1 = p1 - p0,
		s2 = p3 - p2,
		u = p0 - p2;
	float ip = 1.f / (-s2.x * s1.y + s1.x * s2.y),
		s = (-s1.y * u.x + s1.x * u.y) * ip,
		t = (s2.x * u.y - s2.y * u.x) * ip;
	if (s >= 0.f && s <= 1.f && t >= 0.f && t <= 1.f) {
		*i = p0 + (s1 * t);
		return true;
	}
	return false;
}
//2D line segments intersect
inline bool intersect(cv::Point &p0, cv::Point &p1, cv::Point &p2, cv::Point &p3, cv::Point *i) {
	cv::Point s1 = p1 - p0,
		s2 = p3 - p2,
		u = p0 - p2;
	float ip = 1.f / float(-s2.x * s1.y + s1.x * s2.y),
		s = float(-s1.y * u.x + s1.x * u.y) * ip,
		t = float(s2.x * u.y - s2.y * u.x) * ip;
	if (s >= 0.f && s <= 1.f && t >= 0.f && t <= 1.f) {
		*i = p0 + (s1 * t);
		return true;
	}
	return false;
}
//3D lines intersect
inline bool intersect3D(glm::vec3 o1, glm::vec3 d1, glm::vec3 o2, glm::vec3 d2, glm::vec3 &out1, glm::vec3 &out2) {
	float a = glm::dot(d1, d1),
		  b = glm::dot(d1, d2),
		  e = glm::dot(d2, d2),
		  d = a * e - b * b;
	if (d != 0.f) {
		glm::vec3 r = o1 - o2;
		float c = glm::dot(d1, r),
			  f = glm::dot(d2, r),
			  s = (b*f - c * e) / d,
			  t = (a*f - b * c) / d;
		out1 = o1 + d1 * s;
		out2 = o2 + d2 * t;
		return true;
	}
	return false;
}
///////////////////////////////////////////////////////////// Triangle /////////////////////////////////////////////////////////////

//get wuv coords from xyz and triangle, 2D version
inline bool Barycentric(glm::vec2 &p, glm::vec2 &a, glm::vec2 &b, glm::vec2 &c, float &u, float &v, float &w) {
	glm::vec2 v0 = b - a, v1 = c - a, v2 = p - a;
	float iden = 1.f / (v0.x * v1.y - v1.x * v0.y);
	v = (v2.x * v1.y - v1.x * v2.y) * iden;
	if (v < -1.f || v > 1.f) return true;
	w = (v0.x * v2.y - v2.x * v0.y) * iden;
	if (w < 0.f || w > 1.f) return true;
	u = 1.f - v - w;
	if (u < 0.f || u > 1.f) return true;
	return false;
}
//get wuv coords from xyz and triangle, 3D precomputed version
inline bool Barycentric(glm::vec4 &pt, glm::mat4 &clipPrecomp, float &u, float &v, float &w) {
	glm::vec4 e1 = pt - clipPrecomp[2];
	float d20 = glm::dot(e1, clipPrecomp[0]),
		d21 = glm::dot(e1, clipPrecomp[1]);
	v = (clipPrecomp[3][2] * d20 - clipPrecomp[3][1] * d21) * clipPrecomp[3][3];
	if (v < -1.f || v > 1.f) return true;
	w = (clipPrecomp[3][0] * d21 - clipPrecomp[3][1] * d20) * clipPrecomp[3][3];
	if (w < 0.f || w > 1.f) return true;
	u = 1.f - v - w;
	if (u < 0.f || u > 1.f) return true;
	return false;
}

//returns {t, u, v} if refPt is inside v012, according to ray(o,d)
inline glm::vec3 TriRayIntersect(glm::vec3 &o, glm::vec3 &d, glm::vec3 &v1, glm::vec3 &v2, glm::vec3 &v3) {
	glm::vec3 e1, e2, p, q, T, ret;
	float det, inv_det, u, v, t;
	//find vectors for edges
	e1 = v2 - v1;
	e2 = v3 - v1;
	//find det
	p = glm::cross(d, e2);
	det = glm::dot(e1, p);
	//not culling
	if (det < 0.0001)
		return glm::vec3(-1.f);
	//don't late mult
	inv_det = 1.f / det;
	//dist from v1 to origin
	T = o - v1;
	//calculate u
	u = inv_det * glm::dot(T, p);
	//if outside triangle
	if (u < 0.f || u > 1.f)
		return glm::vec3(-1.f);
	//calc v
	q = glm::cross(T, e1);
	v = inv_det * glm::dot(d, q);
	//int lies outisde tri
	if (v < 0.f || u + v > 1.f)
		return glm::vec3(-1.f);
	//calc dist along d to triangle
	t = inv_det * glm::dot(e2, q);
	//return everything
	return glm::vec3(t, u, v);
}

//get xyz coords from uvw and triangle, 2D version
inline void inverseBarycentric(glm::vec3 &wuv, glm::vec2 &a, glm::vec2 &b, glm::vec2 &c, glm::vec2 &xy) {
	xy.x = a.x * wuv.x + b.x * wuv.y + c.x * wuv.z;
	xy.y = a.y * wuv.x + b.y * wuv.y + c.y * wuv.z;
}
//get wuv coords from xyz and triangle
inline void inverseBarycentric(glm::vec3 &wuv, glm::vec3 &a, glm::vec3 &b, glm::vec3 &c, glm::vec3 &xyz) {
	xyz.x = a.x * wuv.x + b.x * wuv.y + c.x * wuv.z;
	xyz.y = a.y * wuv.x + b.y * wuv.y + c.y * wuv.z;
	xyz.z = a.z * wuv.x + b.z * wuv.y + c.z * wuv.z;
}

///////////////////////////////////////////////////////////// Quad /////////////////////////////////////////////////////////////

//this is pretty much numpy "linspace(r,c,r+c,true) - (r/2, c/2)"
inline cv::Mat edgeDistanceCV(int row, int col) {
	cv::Mat I(cv::Size(row, col), CV_32FC2);

	int channels = I.channels();
	int nRows = I.rows;
	int nCols = I.cols * channels;
	int hr = row >> 1;
	int hc = col >> 1;

	int i, j;
	float *p;
	for (i = 0; i < nRows; ++i) {
		p = I.ptr<float>(i);
		for (j = 0; j < nCols - (channels - 1); j += channels) {
			p[j] = float(i - hr);
			p[j + 1] = float((j >> 1) - hc);
		}
	}

	return I;
}
//this is pretty much numpy "linspace(r,c,r+c,true) - (r/2, c/2)"
inline std::vector<float> edgeDistanceSTD(int row, int col) {
	std::vector<float> ret(row*col * 2);

	int hr = row >> 1;
	int hc = col >> 1;

	int i, j;
	for (i = 0; i < row; ++i) {
		for (j = 0; j < col; j++) {
			ret.push_back(float(i - hr));
			ret.push_back(float(j - hc));
		}
	}

	return ret;
}
//generate ray cloud with (flip y, aspect, scale, size) support
inline std::vector<float> edgeDistanceNormalizedSTD(int row, int col) {
	std::vector<float> ret(row * col * 3);
	//for normalization
	int hr = row >> 1, hc = col >> 1, i, j;
	float divR = float(hr), divC = float(hc);
	for (i = 0; i < row; ++i) {
		for (j = 0; j < col; j++) {
			//store vector
			float x = float(i - hr) / divR,
				  y = float(j - hc) / divC;
			ret.push_back(x);
			ret.push_back(y);
			ret.push_back(-1.f);
		}
	}

	return ret;
}

//get axis aligned bounding box
inline void getAABB(std::vector<cv::Point> &in, RectExt &out) {
	int minx = 9999, miny = 9999, maxx = 0, maxy = 0;
	for (int j = 0; j < in.size(); j++) {
		in[j].x < minx ? minx = in[j].x : false;
		in[j].x > maxx ? maxx = in[j].x : false;
		in[j].y < miny ? miny = in[j].y : false;
		in[j].y > maxy ? maxy = in[j].y : false;
	}
	out = RectExt(cv::Point(minx, miny), cv::Point(maxx, maxy));
}
//get axis aligned bounding box
inline void getAABB(glm::vec2 *in, int size, RectExt &out) {
	int minx = 9999, miny = 9999, maxx = 0, maxy = 0;
	for (int j = 0; j < size; ++j) {
		int(in[j].x) < minx ? minx = int(in[j].x) : false;
		int(in[j].x) > maxx ? maxx = int(in[j].x) : false;
		int(in[j].y) < miny ? miny = int(in[j].y) : false;
		int(in[j].y) > maxy ? maxy = int(in[j].y) : false;
	}
	out = RectExt(cv::Point(minx, miny), cv::Point(maxx, maxy));
}

//////////////////////////////////////////////////////// Rendering stuff ////////////////////////////////////////////////////////
struct sph {
	//center Location, Radius
	glm::vec3 l; float r;
	sph(glm::vec3 _l, float _r) : l(_l), r(_r) {}
};
struct pln {
	//Location, Orientation
	glm::vec3 l; glm::mat3 o;
	pln(glm::vec3 _l, glm::mat3 _o) : l(_l), o(_o) {}
};
struct box {
	//Center, Size, Orientation
	glm::vec3 c, s; glm::mat3 o;
	box(glm::vec3 _c, glm::vec3 _s, glm::mat3 _o) : c(_c), s(_s), o(_o) {}
};

static const glm::vec3 v30 = glm::vec3(0.f);
static const glm::vec3 v31 = glm::vec3(1.f);

inline float vec3max(glm::vec3 a) { return glm::max(a.x, glm::max(a.y, a.z)); }
inline float vec3min(glm::vec3 a) { return glm::min(a.x, glm::min(a.y, a.z)); }

inline float smin(float a, float b, float k) {
	float h = glm::clamp(.5f + .5f*(b - a) / k, 0.f, 1.f);
	return glm::mix(b, a, h) - k * h*(1.f - h);
}
inline float smax(float a, float b, float k) {
	return -smin(-a, -b, k);
}

inline float sd(glm::vec3 l, box b) {
	glm::vec3 d = glm::abs(l - b.c) - b.s;
	return glm::min(vec3max(d), 0.f) + glm::length(glm::max(d, 0.f));
}
inline float sd(glm::vec3 l, sph s) {
	glm::vec3 oc = l - s.l;
	return glm::dot(oc, oc) - s.r * s.r;
}
inline float sd(glm::vec3 l, pln p) {
	return glm::dot(p.o[1], l - p.l);
}

inline glm::vec3 nrm(glm::vec3 l, box b) {
	glm::vec3 a = l - b.c;
	return glm::step(b.s*.99999f, glm::abs(a)) * glm::sign(a);
}
inline glm::vec3 nrm(glm::vec3 l, sph s) {
	return (l - s.l) / s.r;
}
inline glm::vec3 nrm(glm::vec3 l, pln p) {
	return p.o[1];
}

inline glm::vec2 map(glm::vec3 l, box b) {
	glm::mat3 o = glm::mat3(v30, nrm(l, b) + .00001f, v30);
	basis(o[1], o[0], o[2]);
	glm::vec3 r = l * o;
	return glm::vec2(r.x, r.z);
}
inline glm::vec2 map(glm::vec3 l, glm::vec3 n, box b) {
	glm::mat3 o = glm::mat3(v30, n + .00001f, v30);
	basis(o[1], o[0], o[2]);
	glm::vec3 r = l * o;
	return glm::vec2(r.x, r.z);
}
inline glm::vec2 map(glm::vec3 l, sph s) {
	glm::vec3 n = nrm(l, s);
	return glm::vec2(glm::atan(n.z, n.x) + M_PI, glm::acos(-n.y)) / glm::vec2(M_PI2, M_PI);
}
inline glm::vec2 map(glm::vec3 l, glm::vec3 n, sph s) {
	return glm::vec2(glm::atan(n.z, n.x) + M_PI, glm::acos(-n.y)) / glm::vec2(M_PI2, M_PI);
}
inline glm::vec2 map(glm::vec3 l, pln p) {
	return glm::vec2(glm::dot(l, p.o[0]), glm::dot(l, p.o[2]));
}

inline glm::vec2 rs(glm::vec3 ro, glm::vec3 rd, box b) {
	glm::vec3 t0 = b.c - ro,
		t1 = (t0 - b.s) / rd,
		t2 = (t0 + b.s) / rd;
	float tn = vec3max(glm::min(t1, t2)),
		  tx = vec3min(glm::max(t1, t2));
	if (tx<tn || tx<0.f) return glm::vec2(0.f);
	return glm::vec2(tn, tx);
}
inline glm::vec2 rs(glm::vec3 ro, glm::vec3 rd, sph s) {
	glm::vec3 oc = ro - s.l;
	float c = glm::dot(oc, oc) - s.r * s.r,
		  b = -glm::dot(oc, rd),
		  h = b * b - c;
	if (h < 0.f) return glm::vec2(0.f);
	h = sqrtf(h);
	return glm::vec2(b - h, b + h);
}
inline glm::vec2 rs(glm::vec3 ro, glm::vec3 rd, pln p) {
	float t = glm::dot(p.o[1], p.l - ro) / glm::dot(p.o[1], rd);
	return glm::vec2(t, t);
}
