
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TERRAIN_FEATURE_FWD_HPP_INCLUDED
#define TERRAIN_FEATURE_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace hex
{

class terrain_feature;
typedef boost::shared_ptr<terrain_feature> terrain_feature_ptr;
typedef boost::shared_ptr<const terrain_feature> const_terrain_feature_ptr;
}

#endif
