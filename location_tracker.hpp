#ifndef LOCATION_TRACKER_HPP_INCLUDED
#define LOCATION_TRACKER_HPP_INCLUDED

#include <math.h>
#include <vector>
#include <iostream>
#include "boost/shared_array.hpp"
#include <GL/gl.h>
#include <GL/glu.h>

#include "raster.hpp"

namespace graphics {

class location_tracker {
public:
	location_tracker();
	void get_bbox(GLfloat *box) const;
	void get_bcircle(GLfloat *circ) const;
	void add_vertex(GLfloat v1, GLfloat v2, GLfloat v3);
	void clear_vertices();
	void update();
private:
	GLfloat box_[4];
	std::vector<boost::shared_array<GLfloat> > vertices_;
};

inline location_tracker::location_tracker() {
	for(int i=0;i<4;++i) {
		box_[i] = 0.0;
	}
}

inline void location_tracker::get_bbox(GLfloat *box) const {
	for(int i =0;i<4;++i) {
		box[i] = box_[i];
	}
}

inline void location_tracker::get_bcircle(GLfloat* bcircle) const {
	bcircle[0] = (box_[0] + box_[2])/2;
	bcircle[1] = (box_[1] + box_[3])/2;
	bcircle[2] = 0;

	for(int i=0;i<2;++i) {
		GLfloat dist_x = (box_[i*2] - bcircle[0]);
		GLfloat dist_y = (box_[i*2+1] - bcircle[1]);
		GLfloat dist = dist_x*dist_x + dist_y*dist_y;
		if(dist > bcircle[2]) {
			bcircle[2] = dist;
		}
	}
	bcircle[2] = sqrt(bcircle[2]);
}


inline void location_tracker::add_vertex(GLfloat v1, GLfloat v2, GLfloat v3) {
	boost::shared_array<GLfloat> my_v(new GLfloat[3]);
	my_v[0] = v1;
	my_v[1] = v2;
	my_v[2] = v3;

	vertices_.push_back(my_v);
}

inline void location_tracker::clear_vertices() {
	vertices_.clear();
}

inline void location_tracker::update() {
	GLdouble model[16], proj[16];
	GLint view[4];

	glGetDoublev(GL_MODELVIEW_MATRIX, model);
	glGetDoublev(GL_PROJECTION_MATRIX, proj);
	glGetIntegerv(GL_VIEWPORT, view);

	/* top left */
	box_[0] = static_cast<GLfloat>(screen_width());
	box_[1] = static_cast<GLfloat>(screen_height());
	/* bottom right */
	box_[2] = 0.0;
	box_[3] = 0.0;

	for(std::vector<boost::shared_array<GLfloat> >::iterator it = vertices_.begin();
	    it != vertices_.end(); ++it) {
		GLdouble v[3];

		gluProject((*it)[0], (*it)[1], (*it)[2], 
			   model, proj, view, v, v+1, v+2);

		if(v[0] < box_[0]) {
			box_[0] = v[0];
		}
		if(v[1] < box_[1]) {
			box_[1] = v[1];
		}
		if(v[0] > box_[2]) {
			box_[2] = v[0];
		}
		if(v[1] > box_[3]) {
			box_[3] = v[1];
		}
	}

	/* screen is "upside down" */
	GLfloat tmp = box_[1];
	box_[1] = screen_height() - box_[3];
	box_[3] = screen_height() - tmp;
}

}

#endif
