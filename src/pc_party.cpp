
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
#include "pathfind.hpp"
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

void pc_party::set_destination(const hex::location& dst)
{
	path_.clear();
	const bool adjacent_only = get_visible_locs().count(dst) && game_world().get_party_at(dst);
	hex::find_path(loc(), dst, *this, &path_, 100000, adjacent_only);
	if(path_.empty() == false && path_.back() == loc()) {
		path_.pop_back();
	}
}

const std::vector<hex::location>* pc_party::get_current_path() const
{
	return path_.empty() ? NULL : &path_;
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
		if(p.get() == this) {
			continue;
		}

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

    bool run, should_pass;
	const hex::DIRECTION dir = keyboard::global_controls.dir(
	               game_world().camera().direction(),
                   run, should_pass);

	if(dir != hex::NULL_DIRECTION) {
		path_.clear();
		const hex::location dst = tile_in_direction(loc(),dir);
		if(movement_cost(loc(),dst) >= 0) {
			set_movement_mode(run ? RUN : WALK);
			move(dir);
			return TURN_COMPLETE;
		}
	} else if(should_pass) {
		path_.clear();
		pass();
		return TURN_COMPLETE;
	}

	if(!path_.empty()) {
		if(path_.back() == loc()) {
			path_.pop_back();
		}

		if(path_.size() > 1 && game_world().get_party_at(path_.back())) {
			const hex::location loc = path_.back();
			set_destination(path_.front());
			if(path_.empty()) {
				path_.push_back(loc);
			}
		}

		if(!path_.empty()) {

			const hex::DIRECTION dir = hex::get_adjacent_direction(loc(), path_.back());
			if(dir == hex::NULL_DIRECTION) {
				path_.clear();
			} else {
				move(dir);
				path_.pop_back();
			}
			return TURN_COMPLETE;
		}
	}

	return TURN_STILL_THINKING;
}

void pc_party::enter_new_world(const world& w)
{
	path_.clear();
}

}
