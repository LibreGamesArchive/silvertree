
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "display_list.hpp"

namespace graphics {

display_list::display_list(GLsizei range)
	: list_(glGenLists(range)), range_(range)
{}

display_list::~display_list()
{
	glDeleteLists(list_,range_);
}

}
