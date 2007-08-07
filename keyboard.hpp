
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef KEYBOARD_HPP_INCLUDED
#define KEYBOARD_HPP_INCLUDED

#include "tile_logic.hpp"

namespace keyboard
{

hex::DIRECTION dir(hex::DIRECTION orientation);
bool run();
bool pass();
		
}

#endif
