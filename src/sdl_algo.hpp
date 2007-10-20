
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SDL_UTILS_HPP_INCLUDED
#define SDL_UTILS_HPP_INCLUDED

#include "surface.hpp"

namespace graphics {

surface get_surface_portion(surface surf, const SDL_Rect& rect);
surface get_non_transparent_portion(surface surf);
SDL_Rect get_non_transparent_rect(surface surf);

}

#endif
