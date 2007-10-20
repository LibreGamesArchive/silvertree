
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "battle_character_pc.hpp"
#include "battle.hpp"
#include "character.hpp"

namespace game_logic
{

battle_character_pc::battle_character_pc(character_ptr ch,
				 const party& p,
                 const hex::location& loc, hex::DIRECTION facing,
				 const hex::gamemap& map, const game_time& time)
  : battle_character(ch,p,loc,facing,map,time)
{
}

bool battle_character_pc::is_human() const
{
	return true;
}

void battle_character_pc::do_turn(battle& b)
{
	b.player_turn(*this);
}

}
