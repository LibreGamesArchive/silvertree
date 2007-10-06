
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef GRAPHICS_LOGIC_HPP_INCLUDED
#define GRAPHICS_LOGIC_HPP_INCLUDED

#include <GL/gl.h>

namespace graphics {

GLfloat calculate_rotation(GLfloat rotate1, GLfloat rotate2,
                           GLfloat ratio);
		
}

#endif
