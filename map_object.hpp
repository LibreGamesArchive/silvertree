
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef MAP_OBJECT_HPP_INCLUDED
#define MAP_OBJECT_HPP_INCLUDED

#include "tile_logic.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/array.hpp>

#include <GL/gl.h>

namespace hex
{

class map_object
{
public:
	typedef boost::array<GLfloat,3> point;

	virtual ~map_object() {}
	virtual void draw() = 0;
	const point& position() const { return point_; }

	GLfloat* position_buffer() { return &point_[0]; }
protected:
	explicit map_object(const GLfloat* p)
	{
		set_position(p);
	}

	void set_position(const GLfloat* buf) {
		std::copy(buf,buf+3,position_buffer());
	}
	
private:
	point point_;
};

typedef boost::shared_ptr<map_object> map_object_ptr;
typedef boost::shared_ptr<const map_object> const_map_object_ptr;
		
}

#endif
