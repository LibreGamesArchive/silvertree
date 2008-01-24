
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef FLOATING_LABEL_HPP_INCLUDED
#define FLOATING_LABEL_HPP_INCLUDED

#include "texture.hpp"

#include <GL/glew.h>

namespace graphics
{

namespace floating_label
{

void add(texture t, const GLfloat* pos, const GLfloat* velocity,
         int ttl);
void clear();

void update_labels();
void draw_labels();

}

}

#endif
