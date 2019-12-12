/*
Takes CV's implementation of Rectangle and adds:
	'right' and 'bottom' public members
	intersect (crop) function
	join (union) function
	translate function
	conversions
*/

#pragma once

#include <opencv2\core.hpp>

class RectExt : public cv::Rect {
public:
	int r, b;
	//construct
	RectExt() { ; }
	RectExt(cv::Rect R) {
		r = R.x + R.width;
		b = R.y + R.height;
	}
	RectExt(int nx, int ny, int nw, int nh) {
		x = nx;		y = ny;
		width = nw;	height = nh;
		r = x + width;	b = y + height;
	}
	RectExt(cv::Point xy, cv::Point rb) {
		x = xy.x;		y = xy.y;
		r = rb.x;		b = rb.y;
		width = abs(r - x);
		height = abs(b - y);
	}
	//operate
	RectExt intersect(RectExt R) {
		if (r < R.x || x > R.r || b < R.y || y > R.b)
			return R;

		int nx = (x > R.x) ? x : R.x, //choose greatest x/y
			ny = (y > R.y) ? y : R.y,
			nw = (r < R.r) ? r - nx : R.r - nx, //least r/b used to calculate w/h
			nh = (b < R.b) ? b - ny : R.b - ny;
		return RectExt(nx, ny, abs(nw), abs(nh));
	}
	RectExt join(RectExt R) {
		int sx = (x < R.x) ? x : R.x, //choose least x/y
			sy = (y < R.y) ? y : R.y,
			sw = (r > R.r) ? r - sx : R.r - sx, //greatest r/b used for w/h
			sh = (b > R.b) ? b - sy : R.b - sy;
		return RectExt(sx, sy, sw, sh);
	}
	//move rect
	void translate(int Lx, int Ly) {
		x += Lx; y += Ly;
		r += Lx; b += Ly;
	}
	//move rect
	void translate(cv::Point L) {
		x += L.x; y += L.y;
		r += L.x; b += L.y;
	}
	//change rect size
	void resize(int Lw, int Lh) {
		width = Lw; height = Lh;
		r = x + width; b = y + height;
	}
	//scale rect size only
	void scale(float s) {
		width = int(float(width)*s); height = int(float(height)*s);
		r = x + width; b = y + height;
	}
	//scale rect size and location centered
	void scaleCentered(float s) {
		int lw = width, lh = height;
		width = int(float(width)*s); height = int(float(height)*s);
		x -= (width - lw) >> 1; y -= (height - lh) >> 1;
		r = x + width; b = y + height;
	}

	//some nice things
	cv::Point toPointxy() {
		return cv::Point(x, y);
	}
	cv::Point toPointrb() {
		return cv::Point(r, b);
	}
	cv::Point toPointwh() {
		return cv::Point(width, height);
	}
};