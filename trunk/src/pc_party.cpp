
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "foreach.hpp"
#include "global_game_state.hpp"
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

wml::node_ptr pc_party::write() const
{
	wml::node_ptr res = party::write();
	res->set_attr("controller", "human");
	return res;
}

void pc_party::encounter(party& p, const std::string& type)
{
	assert(dynamic_cast<pc_party*>(&p) == NULL);
	p.encounter(*this, type);
}

party::TURN_RESULT pc_party::do_turn()
{
	std::vector<const_party_ptr> now_visible;
	get_visible_parties(now_visible);
	foreach(const_party_ptr& p, now_visible) {
		if(std::find(seen_.begin(), seen_.end(), p) == seen_.end()) {
			map_formula_callable_ptr callable(new map_formula_callable);
			callable->add("world", variant(&game_world()))
			         .add("var", variant(&global_game_state::get().get_variables()))
					 .add("pc", variant(this))
					 .add("npc", variant(p.get()));
			game_world().fire_event("sight", *callable);
		}
	}

	seen_.swap(now_visible);

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
