
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef ENCOUNTER_HPP_INCLUDED
#define ENCOUNTER_HPP_INCLUDED

#include "gamemap.hpp"
#include "party.hpp"

namespace game_logic
{

void handle_encounter(party_ptr p1, party_ptr p2,
                      const hex::gamemap& map);
bool play_battle(party_ptr p1, party_ptr p2, const std::vector<character_ptr>& c1, const std::vector<character_ptr>& c2, const hex::location& loc);

}

#endif
