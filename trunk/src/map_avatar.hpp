
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef MAP_AVATAR_HPP_INCLUDED
#define MAP_AVATAR_HPP_INCLUDED

#include "map_object.hpp"
#include "model_fwd.hpp"
#include "wml_node.hpp"

namespace hex
{

class map_avatar;

typedef boost::shared_ptr<map_avatar> map_avatar_ptr;
typedef boost::shared_ptr<const map_avatar> const_map_avatar_ptr;

class map_avatar : public map_object
{
public:
	static map_avatar_ptr create(wml::const_node_ptr node,
	                             const GLfloat* pos);
	bool valid() const;
	void write(wml::node_ptr node) const;
	void set_rotation(GLfloat rotate);
	void draw();
private:
	map_avatar(wml::const_node_ptr node, const GLfloat* pos);

	graphics::const_model_ptr model_;
	std::string model_id_;
	std::string image_;
	GLfloat rotate_;
};

}

#endif
