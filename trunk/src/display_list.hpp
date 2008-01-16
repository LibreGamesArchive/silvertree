
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef DISPLAY_LIST_HPP_INCLUDED
#define DISPLAY_LIST_HPP_INCLUDED

#include <GL/gl.h>

namespace graphics {

class display_list
{
public:
	explicit display_list(GLsizei range=1);
	~display_list();

	GLuint operator[](GLsizei index) const {
		return list_ + index;
	}

	GLuint get() const { return list_; }

	GLsizei size() const { return range_; }
private:
	GLuint list_;
	GLsizei range_;
};

}

#endif
