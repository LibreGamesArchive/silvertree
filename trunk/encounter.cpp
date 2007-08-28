
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

	if(!p1->is_enemy(*p2)) {
		p2->friendly_encounter(*p1);
		return;
	}

	boost::shared_ptr<hex::gamemap> battle_map =
	            generate_battle_map(map,p1->loc());

	int xp1 = 0, xp2 = 0;

	std::vector<battle_character_ptr> chars;
	for(std::vector<character_ptr>::const_iterator i =
	    p1->members().begin(); i != p1->members().end(); ++i) {
		hex::location loc(2 + i - p1->members().begin(),2);
		chars.push_back(battle_character::make_battle_character(
		                    *i,*p1,loc,hex::NORTH,*battle_map,
							p1->game_world().current_time()));
		xp1 += (*i)->level()*10;
	}

	for(std::vector<character_ptr>::const_iterator i =
	    p2->members().begin(); i != p2->members().end(); ++i) {
		hex::location loc(2 + i - p2->members().begin(),8);
		chars.push_back(battle_character::make_battle_character(
		                    *i,*p2,loc,hex::NORTH,*battle_map,
							p2->game_world().current_time()));
		xp2 += (*i)->level()*10;
	}

	battle b(chars,*battle_map);
	b.play();

	if(p1->is_destroyed() || p2->is_destroyed()) {
		if(p1->is_destroyed()) {
			std::cerr << "SWAP!\n";
			std::swap(p1,p2);
			std::swap(xp1,xp2);
		}

		if(p1->is_human_controlled()) {
			std::cerr << "INFO: " << p1.get() << " " << xp2 << " " << p2->money() << "\n";
			game_dialogs::post_battle_dialog d(p1, xp2, p2->money());
			d.show();
		}
	}

	if(p1->is_destroyed() == false) {
		foreach(const character_ptr& c, p1->members()) {
			if(!c->dead()) {
				c->award_experience(xp2);
			} else {
				c->set_to_near_death();
			}
		}
	}

	if(p2->is_destroyed() == false) {
		foreach(const character_ptr& c, p2->members()) {
			if(!c->dead()) {
				c->award_experience(xp1);
			} else {
				c->set_to_near_death();
			}
		}
	}
}
		
}
