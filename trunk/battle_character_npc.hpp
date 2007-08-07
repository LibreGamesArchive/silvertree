
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BATTLE_CHARACTER_NPC_INCLUDED
#define BATTLE_CHARACTER_NPC_INCLUDED

#include "battle_character.hpp"

namespace game_logic
{

class battle_character_npc : public battle_character
{
public:
	battle_character_npc(character_ptr ch, const_party_ptr p,
	                    const hex::location& loc, hex::DIRECTION facing,
					    const hex::gamemap& map, const game_time& time);

private:
	bool is_human() const;
	void do_turn(battle& b);
};
		
}

#endif
