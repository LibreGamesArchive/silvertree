#ifndef FRUSTUM_HPP_INCLUDED
#define FRUSTUM_HPP_INCLUDED

#include "tile.hpp"

namespace hex {

class frustum {
public:
	/* point containment */
	bool contains(const tile::point& p);
	/* sphere intersection */
	bool intersects(const tile::point& p, GLfloat radius);
	/* tile intersection */
	bool intersects(const tile& t);
	/* frustum filling the given space in the world */
	void set_volume_world_space(GLfloat bsphere);
	/* frustum filling the given space in clip space */
	void set_volume_clip_space(GLfloat xmin, GLfloat xmax,
				   GLfloat ymin, GLfloat ymax, 
				   GLfloat zmin, GLfloat zmax);
	/* class init per frame */
	static void initialize();
private:
	Uint32 calculate_misses(const tile::point &p);
	GLfloat planes_[6*4];
	static GLfloat comb_[16];
};

inline bool frustum::contains(const tile::point& p) {
	/* for each plane */
	for(int i=0;i<6;++i) {
		GLfloat d = planes_[i*4+0]*p.x + planes_[i*4+1]*p.y + planes_[i*4+2]*p.height + planes_[i*4+3];
		if(d <= 0) {
			/* this plane has point on wrong side */
			return false;
		}
	}
	/* all planes have point on correct side */
	return true;
}

inline bool frustum::intersects(const tile::point& p, GLfloat radius) {
	for(int i=0;i<6;++i) {
		GLfloat d = planes_[i*4+0]*p.x + planes_[i*4+1]*p.y + planes_[i*4+2]*p.height + planes_[i*4+3] + radius;
		if(d <= 0) {
			return false;
		}
	}
	return true;
}

inline Uint32 frustum::calculate_misses(const tile::point& p) {
	Uint32 plane_misses = 0;
	for(int j=0;j<6;++j) {
		GLfloat d = planes_[j*4+0]*p.x + planes_[j*4+1]*p.y + planes_[j*4+2]*p.height + planes_[j*4+3];
		if(d <= 0) {
			plane_misses |= 1<<j;
		}
	}
	return plane_misses;
}

inline bool frustum::intersects(const tile& t) {
	Uint32 plane_misses[7];
	for(int i=0;i<6;++i) {
		plane_misses[i] = calculate_misses(t.corners_[i]);
	}
	plane_misses[6] = calculate_misses(t.center_);

	Uint32 all_plane_miss = static_cast<Uint32>(-1);
	for(int i=0;i<7;++i) {
		all_plane_miss &= plane_misses[i];
	}
	return all_plane_miss == 0;
}

}

#endif
