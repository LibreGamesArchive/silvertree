
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <assert.h>
#include <iostream>

#include "battle.hpp"
#include "battle_character_npc.hpp"
#include "battle_move.hpp"
#include "character.hpp"
#include "foreach.hpp"

namespace game_logic
{

battle_character_npc::battle_character_npc(character_ptr ch, const party& p,
                 const hex::location& loc, hex::DIRECTION facing,
				 const hex::gamemap& map, const game_time& time)
  : battle_character(ch,p,loc,facing,map,time)
{
}

bool battle_character_npc::is_human() const
{
	return false;
}

namespace {
int rate_attack_stats(const battle::attack_stats& stats) {
	if(stats.attack + stats.defense > 0) {
		return ((stats.attack*100)/(stats.attack+stats.defense)) * stats.damage;
	} else {
		return 0;
	}
}
}

void battle_character_npc::do_turn(battle& b)
{
	const std::vector<const_battle_move_ptr>& moves = get_character().battle_moves();
	const_battle_move_ptr best_move;
	hex::location best_dst;
	battle_character_ptr best_target;
	route best_route;
	int best_rating = -1;
	foreach(const const_battle_move_ptr& m, moves) {
		if(m->can_attack() == false) {
			continue;
		}

		move_map movements;
		if(m->max_moves() > 0) {
			get_possible_moves(movements,*m,b.participants());
		} else {
			movements[loc()] = route();
		}

		typedef std::pair<hex::location,route> move_pair;
		foreach(const move_pair& movement, movements) {
			const hex::location& dst = movement.first;
			foreach(const battle_character_ptr& c, b.participants()) {
				if(!is_enemy(*c) || !can_attack(*c,b.participants(),dst)) {
					continue;
				}

				const int rating = rate_attack_stats(
				          b.get_attack_stats(*this,*c,*m,NULL,dst));
				if(best_rating == -1 || rating > best_rating) {
					best_move = m;
					best_dst = dst;
					best_target = c;
					best_rating = rating;
					best_route = movement.second;
				}
			}
		}
	}

	if(best_target) {
		if(best_dst.valid() && best_dst != loc()) {
			b.move_character(*this,best_route);
		}

		b.attack_character(*this,*best_target,*best_move);
		return;
	}

	foreach(const const_battle_move_ptr& m, moves) {
		if(m->must_attack() || m->max_moves() == 0) {
			continue;
		}

		battle_character_ptr target;
		int closest = -1;
		foreach(const battle_character_ptr& c, b.participants()) {
			if(!is_enemy(*c)) {
				continue;
			}

			if(closest == -1 || distance_between(loc(),c->loc()) < closest) {
				closest = distance_between(loc(),c->loc());
				target = c;
			}
		}

		assert(target);

		closest = -1;
		move_map movements;
		route best_move;
		get_possible_moves(movements,*m,b.participants());
		typedef std::pair<hex::location,route> move_pair;
		foreach(const move_pair& movement, movements) {
			const hex::location& dst = movement.first;
			if(closest == -1 || distance_between(dst,target->loc()) < closest) {
				closest = distance_between(dst,target->loc());
				best_move = movement.second;
			}
		}

		if(closest < distance_between(loc(),target->loc())) {
			b.move_character(*this, best_move);
			return;
		}
	}

	set_time_until_next_move(1);
}
		
}
