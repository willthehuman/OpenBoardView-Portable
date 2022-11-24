#include "imgui/imgui.h"
#include <cmath>
#include <iostream>
#include <climits>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "vectorhulls.h"

void VHRotateV(double *px, double *py, double ox, double oy, double theta) {
	double tx, ty, ttx, tty;

	tx = *px - ox;
	ty = *py - oy;

	// With not a lot of parts on the boards, we can get away with using the
	// precision trig functions, might have to change to LUT based later.
	ttx = tx * cos(theta) - ty * sin(theta);
	tty = tx * sin(theta) + ty * cos(theta);

	*px = ttx + ox;
	*py = tty + oy;
}

ImVec2 VHRotateV(ImVec2 v, ImVec2 o, double theta) {
	double tx, ty, ttx, tty;

	tx = v.x - o.x;
	ty = v.y - o.y;

	// With not a lot of parts on the boards, we can get away with using the
	// precision trig functions, might have to change to LUT based later.
	ttx = tx * cos(theta) - ty * sin(theta);
	tty = tx * sin(theta) + ty * cos(theta);

	return ImVec2(ttx + o.x, tty + o.y);
}

ImVec2 VHRotateV(ImVec2 v, double theta) {
	double nx, ny;

	nx = v.x * cos(theta) - v.y * sin(theta);
	ny = v.x * sin(theta) + v.y * cos(theta);

	return ImVec2(nx, ny);
}

double VHAngleToX(ImVec2 a, ImVec2 b) {
	return atan2((b.y - a.y), (b.x - a.x));
}

std::array<ImVec2, 4> VHMBBCalculate(std::vector<ImVec2> hull, double psz) {
	std::array<ImVec2, 4> box;

	double mbAngle = 0, cumulative_angle = 0;
	double mbArea = DBL_MAX; // fake area to initialise
	ImVec2 mbb, mba, origin; // Box bottom left, box top right

	// Find the lowest hull point, if it's below the x-axis just bring it up to
	// compensate
	// NOTE: we're not modifying the actual hull point, just a copy
	origin.x = DBL_MAX;
	origin.y = DBL_MAX;

	// find bottom corner
	for (size_t i = 0; i < hull.size(); i++) {
		if (hull[i].y < origin.y) origin.y = hull[i].y;
		if (hull[i].x < origin.x) origin.x = hull[i].x;
	}

	// transpose
	for (size_t i = 0; i < hull.size(); i++) {
		hull[i].x -= origin.x;
		hull[i].y -= origin.y;
	}

	// rotate hull on each side and work out the smallest area
	for (size_t i = 0; i < hull.size(); i++) {
		int ni = i + 1;
		double area;

		ImVec2 current = hull[i];
		ImVec2 next    = hull[ni % hull.size()];

		double angle = VHAngleToX(current, next); // angle formed between current and next hull points;
		cumulative_angle += angle;

		double top, bot, left, right; // bounding rect limits
		top = right = DBL_MIN;
		bot = left = DBL_MAX;

		for (size_t x = 0; x < hull.size(); x++) {
			ImVec2 rp = VHRotateV(hull[x], -angle);

			hull[x] = rp;

			if (rp.y > top) top     = rp.y;
			if (rp.y < bot) bot     = rp.y;
			if (rp.x > right) right = rp.x;
			if (rp.x < left) left   = rp.x;
		}
		area = (right - left) * (top - bot);

		if (area < mbArea) {
			mbArea  = area;
			mbAngle = cumulative_angle; // total angle we've had to rotate the board
			                            // to get to this orientation;
			mba = ImVec2(left, bot);
			mbb = ImVec2(right, top);
		}
	} // for all points on hull

	// expand by pin size
	mba.x -= psz;
	mba.y -= psz;
	mbb.x += psz;
	mbb.y += psz;

	// Form our rectangle, has to be all 4 points as it's a polygon now that'll be
	// rotated
	box[0] = VHRotateV(mba, +mbAngle);
	box[1] = VHRotateV(ImVec2(mbb.x, mba.y), +mbAngle);
	box[2] = VHRotateV(mbb, +mbAngle);
	box[3] = VHRotateV(ImVec2(mba.x, mbb.y), +mbAngle);

	// Transpose MBB back
	for (size_t i = 0; i < 4; i++) {
		box[i].x += origin.x;
		box[i].y += origin.y;
	}

	return box;
}

// To find orientation of ordered triplet (p, q, r).
// The function returns following values
// 0 --> p, q and r are colinear
// 1 --> Clockwise
// 2 --> Counterclockwise
int VHConvexHullOrientation(ImVec2 p, ImVec2 q, ImVec2 r) {

	int val = trunc(((q.y - p.y) * (r.x - q.x) - (q.x - p.x) * (r.y - q.y)));

	if (val == 0) return 0;   // colinear
	return (val > 0) ? 1 : 2; // clock or counterclock wise
}

std::vector<ImVec2> VHConvexHull(const std::vector<ImVec2> &points) {
	std::vector<ImVec2> hull;

	// There must be at least 3 points
	if (points.size() < 3) return hull;

	// Find the leftmost point
	int l = 0;
	for (size_t i = 1; i < points.size(); i++) {
		if (points[i].x < points[l].x) {
			l = i;
		}
	}

	// Start from leftmost point, keep moving counterclockwise
	// until reach the start point again.  This loop runs O(h)
	// times where h is number of points in result or output.
	int p = l, q;
	do {
		// Add current point to result
		//		hull[hpc] = CoordToScreen(points[p].x, points[p].y);
		hull.push_back({points[p].x, points[p].y});

		// Search for a point 'q' such that orientation(p, x,
		// q) is counterclockwise for all points 'x'. The idea
		// is to keep track of last visited most counterclock-
		// wise point in q. If any point 'i' is more counterclock-
		// wise than q, then update q.
		q = (p + 1) % points.size();
		for (size_t i = 0; i < points.size(); i++) {
			// If i is more counterclockwise than current q, then update q
			if (VHConvexHullOrientation(points[p], points[i], points[q]) == 2) {
				q = i;
			}
		}

		// Now q is the most counterclockwise with respect to p
		// Set p as q for next iteration, so that q is added to
		// result 'hull'
		p = q;

	} while ((p != l) && (hull.size() < points.size())); // While we don't come to first point

	return hull;
}

int VHTightenHull(ImVec2 hull[], int n, double threshold) {
	// theory: circle the hull, compare 3 points at a time, if the mid point is
	// sub-angular then make it equal the first point and move to the 3rd.
	int i, ni;
	ImVec2 *a, *b, *c;
	double a1, a2, ad;
	// First cycle, we look for sub-threshold 2-segment runs
	for (i = 0; i < n; i++) {
		a = &(hull[i]);
		b = &(hull[(i + 1) % n]);
		c = &(hull[(i + 2) % n]);

		a1 = VHAngleToX(*a, *b);
		a2 = VHAngleToX(*b, *c);
		if (a1 > a2)
			ad = a1 - a2;
		else
			ad = a2 - a1;

		if (ad < threshold) {
			//			fprintf(stderr,"angle below threshold
			//(%0.2f)\n", ad);
			*b = *a;
		}
	} // end first cycle

	// Second cycle, we compact the hull
	int output_index = 0;
	i                = 0;
	while (i < n) {
		ni = (i + 1) % n;
		if ((hull[i].x == hull[ni].x) && (hull[i].y == hull[ni].y)) {
			// match found, discard one
			i++;
			continue;
		}

		hull[output_index] = hull[i];
		output_index++;
		i++;
	}

	return output_index;
}

bool GetIntersection(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, ImVec2 *i) {

	ImVec2 s1, s2;
	s1.x = p1.x - p0.x;
	s1.y = p1.y - p0.y;
	s2.x = p3.x - p2.x;
	s2.y = p3.y - p2.y;

	double s = (-s1.y * (p0.x - p2.x) + s1.x * (p0.y - p2.y)) / (-s2.x * s1.y + s1.x * s2.y);
	double t = (s2.x * (p0.y - p2.y) - s2.y * (p0.x - p2.x)) / (-s2.x * s1.y + s1.x * s2.y);

	if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
		// Collision detected
		//
		if (i) {
			i->x = p0.x + (t * s1.x);
			i->y = p0.y + (t * s1.y);
		}
		return true;
	}

	return false; // No collision
}
