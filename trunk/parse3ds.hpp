
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef PARSE_3DS_HPP_INCLUDED
#define PARSE_3DS_HPP_INCLUDED

#include "model_fwd.hpp"

namespace graphics
{

model_ptr parse3ds(const char* i1, const char* i2);

struct parse3ds_error {};
		
}

#endif
