
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "map_avatar.hpp"
#include "model.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"

#include <GL/gl.h>
#include <iostream>

namespace hex
{

map_avatar_ptr map_avatar::create(wml::const_node_ptr node,
                                  const GLfloat* pos)
{
	return map_avatar_ptr(new map_avatar(node,pos));
}

map_avatar::map_avatar(wml::const_node_ptr node, const GLfloat* pos)
    : map_object(pos),
      model_id_((*node)["model"]),
	  image_((*node)["image"]),
	  rotate_(0.0)
{
	if((*node)["model"].empty() == false) {
      model_ = graphics::model::get_model((*node)["model"]);
	}
}

bool map_avatar::valid() const
{
	return model_ || !image_.empty();
}

void map_avatar::write(wml::node_ptr node) const
{
	node->set_attr("model", model_id_);
	node->set_attr("image", image_);
}

void map_avatar::set_rotation(GLfloat rotate)
{
	rotate_ = rotate;
}

void map_avatar::draw()
{
	glPushMatrix();
	glTranslatef(position()[0],position()[1],position()[2]);
	glRotatef(rotate_,0.0,0.0,1.0);

	if(model_) {
		model_->draw();
	} else {
		graphics::surface surf = graphics::surface_cache::get(image_);
		if(surf.get()) {
			graphics::texture::get(surf).set_as_current_texture();
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
		}
	}

	glPopMatrix();
}

}
