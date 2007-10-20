
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include <algorithm>
#include <cmath>
#include <iostream>

#include "graphics_logic.hpp"

namespace graphics {

GLfloat calculate_rotation(GLfloat rotate1, GLfloat rotate2,
                           GLfloat ratio)
{
	ratio = std::max<GLfloat>(ratio,0.0);
	ratio = std::min<GLfloat>(ratio,1.0);

	if(std::abs(rotate2 - rotate1) > 180.0) {
		if(rotate1 < rotate2) {
			rotate1 += 360.0;
		} else {
			rotate2 += 360.0;
		}
	}

	return rotate2*ratio + rotate1*(1.0 - ratio);
}

}
