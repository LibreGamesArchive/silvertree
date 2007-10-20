
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BATTLE_MAP_GENERATOR_HPP_INCLUDED
#define BATTLE_MAP_GENERATOR_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace hex
{

class gamemap;
class location;

}

namespace game_logic
{
	boost::shared_ptr<hex::gamemap> generate_battle_map(
	         const hex::gamemap& world_map, const hex::location& loc);
}

#endif
