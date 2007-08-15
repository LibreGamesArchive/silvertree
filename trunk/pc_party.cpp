
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "keyboard.hpp"
#include "pc_party.hpp"
#include "tile_logic.hpp"
#include "world.hpp"

#include <iostream>

namespace game_logic
{

pc_party::pc_party(wml::const_node_ptr node, world& game_world)
   : party(node,game_world)
{
}

void pc_party::friendly_encounter(party& p)
{
	assert(dynamic_cast<pc_party*>(&p) == NULL);
	p.friendly_encounter(*this);
}

party::TURN_RESULT pc_party::do_turn()
{
	const hex::DIRECTION dir = keyboard::dir(
	               game_world().camera().direction());

	if(dir != hex::NULL_DIRECTION) {
		const hex::location dst = tile_in_direction(loc(),dir);
		if(movement_cost(loc(),dst) >= 0) {
			set_movement_mode(keyboard::run() ? RUN : WALK);
			move(dir);
			return TURN_COMPLETE;
		}
	} else if(keyboard::pass()) {
		pass();
		return TURN_COMPLETE;
	}
	
	return TURN_STILL_THINKING;
}

}
