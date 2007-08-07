
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "camera.hpp"
#include "floating_label.hpp"
#include "foreach.hpp"

#include <algorithm>
#include <iostream>
#include <boost/shared_ptr.hpp>

namespace graphics
{

namespace floating_label
{

namespace
{

struct label
{
	texture tex;
	GLfloat pos[3];
	GLfloat vel[3];
	int ttl;
};

typedef boost::shared_ptr<label> label_ptr;
typedef boost::shared_ptr<const label> const_label_ptr;

std::vector<label_ptr> labels;

bool label_dead(const label_ptr& lb) {
	return lb->ttl <= 0;
}

}

void add(texture t, const GLfloat* pos, const GLfloat* vel, int ttl)
{
	label_ptr lb(new label);
	lb->tex = t;
	std::copy(pos,pos+3,lb->pos);
	std::copy(vel,vel+3,lb->vel);
	lb->ttl = ttl;
	labels.push_back(lb);
}

void clear()
{
	labels.clear();
}

void update_labels()
{
	foreach(const label_ptr& lb, labels) {
		lb->ttl--;
		for(int n = 0; n != 3; ++n) {
			lb->pos[n] += lb->vel[n];
		}
	}

	labels.erase(std::remove_if(labels.begin(),labels.end(),label_dead),labels.end());
}

void draw_labels()
{
	hex::camera* cam = hex::camera::current_camera();
	foreach(const label_ptr& lb, labels) {
		glPushMatrix();
		const texture& t = lb->tex;
		glTranslatef(lb->pos[0],lb->pos[1],lb->pos[2]);
		if(cam) {
			const GLfloat rotate = cam->current_rotation();
			glRotatef(-rotate,0.0,0.0,1.0);
		}
		t.set_as_current_texture();
		glBegin(GL_QUADS);
		graphics::texture::set_coord(0.0,0.0);
		glVertex3f(-0.5,0.0,1.0);
		graphics::texture::set_coord(1.0,0.0);
		glVertex3f(0.5,0.0,1.0);
		graphics::texture::set_coord(1.0,1.0);
		glVertex3f(0.5,0.0,0.0);
		graphics::texture::set_coord(0.0,1.0);
		glVertex3f(-0.5,0.0,0.0);
		glEnd();
		glPopMatrix();
	}
}

}

}
