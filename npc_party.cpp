
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "battle.hpp"
#include "battle_character.hpp"
#include "battle_map_generator.hpp"
#include "global_game_state.hpp"
#include "message_dialog.hpp"
#include "foreach.hpp"
#include "gamemap.hpp"
#include "npc_party.hpp"
#include "shop_dialog.hpp"
#include "tile_logic.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "world.hpp"

#include <cstdlib>
#include <iostream>

namespace game_logic
{

npc_party::npc_party(wml::const_node_ptr node, world& game_world)
    : party(node,game_world), aggressive_(wml::get_bool(node,"aggressive")),
      rest_(wml::get_bool(node,"rest",false))
{
	const std::string& dest_chooser = node->attr("destination_chooser");
	if(!dest_chooser.empty()) {
		next_destination_.reset(new formula(dest_chooser));
	}

	dialog_ = node->get_child("dialog");
	if(wml::const_node_ptr dst = node->get_child("destination")) {
		current_destination_ = hex::location(
		             wml::get_attr<int>(dst,"x",-1),
		             wml::get_attr<int>(dst,"y",-1));
	}

	wml::node::const_child_range range = node->get_child_range("wander");
	while(range.first != range.second) {
		const wml::const_node_ptr& n = range.first->second;
		wander_between_.push_back(hex::location(
		     wml::get_attr<int>(n,"x"), wml::get_attr<int>(n,"y")));
		std::cerr << "wander (" << wander_between_.back().x() << "," << wander_between_.back().y() << ")\n";
		++range.first;
	}
}

wml::node_ptr npc_party::write() const
{
	wml::node_ptr res = party::write();
	res->set_attr("aggressive", aggressive_ ? "yes" : "no");
	res->set_attr("rest", rest_ ? "yes" : "no");
	if(dialog_) {
		res->add_child(wml::deep_copy(dialog_));
	}

	if(map().is_loc_on_map(current_destination_)) {
		res->add_child(hex::write_location("destination", current_destination_));
	}

	foreach(const hex::location& loc, wander_between_) {
		res->add_child(hex::write_location("wander", loc));
	}

	if(next_destination_) {
		res->set_attr("destination_chooser", next_destination_->str());
	}

	return res;
}

void npc_party::encounter(party& p, const std::string& type)
{
	if(!p.is_human_controlled()) {
		return;
	}

	map_formula_callable_ptr callable(new map_formula_callable);
	callable->add("pc", variant(&p))
	         .add("npc", variant(this))
	         .add("world", variant(&game_world()))
	         .add("var", variant(&global_game_state::get().get_variables()));
	game_world().fire_event(type, *callable);
}

void npc_party::set_value(const std::string& key, const variant& value)
{
	if(key == "destination") {
		const hex::location* loc = dynamic_cast<const hex::location*>(value.as_callable());
		if(!loc) {
			std::cerr << "ERROR: npc_party::set_value: location expected, but type is not a location\n";
			return;
		}

		current_destination_ = *loc;
	}
	
	return party::set_value(key, value);
}

party::TURN_RESULT npc_party::do_turn()
{
  //const int start_ticks = SDL_GetTicks();
	std::vector<const_party_ptr> parties;
	if(aggressive_) {
		get_visible_parties(parties);
	}

	hex::location target;
	int closest = -1;
	foreach(const const_party_ptr& party, parties) {
		if(is_enemy(*party) && (closest == -1 || hex::distance_between(loc(),party->loc()) < closest)) {
			closest = hex::distance_between(loc(),party->loc());
			target = party->loc();
			current_destination_ = target;
			break;
		}
	}

	if(current_destination_ == loc()) {
		current_destination_ = hex::location();
	}

	if(!map().is_loc_on_map(current_destination_)) {
		choose_new_destination();
	}

	if(!map().is_loc_on_map(target) &&
	   map().is_loc_on_map(current_destination_) && !rest_) {
		target = current_destination_;
	}

	bool party_refreshed = true;
	foreach(const const_character_ptr& c, members()) {
		if(c->fatigue() > 0) {
			party_refreshed = false;
		}

		if(c->fatigue() >= c->stamina()) {
			rest_ = true;
			break;
		}
	}

	if(party_refreshed) {
		rest_ = false;
	}

	if(map().is_loc_on_map(target)) {
		const int current_distance = hex::distance_between(loc(),target);
		int best = -1;
		hex::DIRECTION dir = hex::NULL_DIRECTION;
		hex::location adj[6];
		hex::get_adjacent_tiles(loc(),adj);
		for(int n = 0; n != 6; ++n) {
			if(map().is_loc_on_map(adj[n]) == false) {
				continue;
			}

			if(hex::distance_between(adj[n],target) >= current_distance) {
				continue;
			}

			std::vector<const_party_ptr> parties_at;
			game_world().get_parties_at(adj[n], parties_at);
			if(!parties_at.empty()) {
				continue;
			}

			const int cost = movement_cost(loc(),adj[n]);
			if(cost != -1 && (best == -1 || cost < best)) {
				best = movement_cost(loc(),adj[n]);
				dir = hex::DIRECTION(n);
			}
		}

		if(dir != hex::NULL_DIRECTION) {
			move(dir);
		} else {
			choose_new_destination();
			pass();
		}

		//const int time_taken = SDL_GetTicks() - start_ticks;
		//std::cerr << "AI took " << time_taken << "\n";
		return TURN_COMPLETE;
	}

	pass();
	return TURN_COMPLETE;
}

void npc_party::choose_new_destination()
{
	if(next_destination_) {
		variant res = next_destination_->execute(*this);
		const hex::location* loc = dynamic_cast<const hex::location*>(res.as_callable());
		if(loc) {
			current_destination_ = *loc;
			return;
		}
	}

	if(!wander_between_.empty()) {
		current_destination_ = wander_between_[rand()%wander_between_.size()];
	}
}

}
