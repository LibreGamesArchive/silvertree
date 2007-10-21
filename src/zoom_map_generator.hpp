/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef ZOOM_MAP_GENERATOR_HPP_INCLUDED
#define ZOOM_MAP_GENERATOR_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "gamemap.hpp"

namespace hex
{

boost::shared_ptr<gamemap> generate_zoom_map(
    const gamemap& input, const location& top_left, const location& src_dim,
	const location& dim, double height_scale, double height_randomness);

}

#endif
