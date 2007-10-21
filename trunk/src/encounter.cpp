
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
#include "character.hpp"
#include "dialog.hpp"
#include "encounter.hpp"
#include "foreach.hpp"
#include "post_battle_dialog.hpp"
#include "world.hpp"

#include <iostream>

namespace game_logic
{

void handle_encounter(party_ptr p1, party_ptr p2,
                      const hex::gamemap& map)
{
	if(!p1->is_human_controlled() && !p2->is_human_controlled()) {
		return;
	}

	p2->encounter(*p1, "encounter");

	if(!p1->is_enemy(*p2)) {
		return;
	}

	boost::shared_ptr<hex::gamemap> battle_map =
	            generate_battle_map(map,p1->loc());

	int xp1 = 0, xp2 = 0;

	std::vector<battle_character_ptr> chars;
	for(std::vector<character_ptr>::const_iterator i =
	    p1->members().begin(); i != p1->members().end(); ++i) {
		hex::location loc(22 + i - p1->members().begin(),22);
		chars.push_back(battle_character::make_battle_character(
		                    *i,*p1,loc,hex::NORTH,*battle_map,
							p1->game_world().current_time()));
		xp1 += (*i)->level()*10;
	}

	for(std::vector<character_ptr>::const_iterator i =
	    p2->members().begin(); i != p2->members().end(); ++i) {
		hex::location loc(22 + i - p2->members().begin(),28);
		chars.push_back(battle_character::make_battle_character(
		                    *i,*p2,loc,hex::NORTH,*battle_map,
							p2->game_world().current_time()));
		xp2 += (*i)->level()*10;
	}

	battle b(chars,*battle_map);
	b.play();

	if(p1->is_destroyed() || p2->is_destroyed()) {
		if(p1->is_destroyed()) {
			std::swap(p1,p2);
			std::swap(xp1,xp2);
		}

		if(p1->is_human_controlled()) {
			game_dialogs::post_battle_dialog d(p1, xp2, p2->money());
			d.show();
			p2->encounter(*p1, "win_battle");
		} else {
			p2->encounter(*p1, "lose_battle");
		}
	}

	if(p1->is_destroyed() == false) {
		foreach(const character_ptr& c, p1->members()) {
			if(!c->dead()) {
				//now gets awarded in the post battle dialog, but
				//later when there are NPC vs NPC fights we should still
				//award experience to NPCs here
				//c->award_experience(xp2);
			} else {
				c->set_to_near_death();
			}
		}
	}

	if(p2->is_destroyed() == false) {
		foreach(const character_ptr& c, p2->members()) {
			if(!c->dead()) {
				//c->award_experience(xp1);
			} else {
				c->set_to_near_death();
			}
		}
	}
}

}
