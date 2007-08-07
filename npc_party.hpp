
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef NPC_PARTY_HPP_INCLUDED
#define NPC_PARTY_HPP_INCLUDED

#include <vector>

#include "party.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class npc_party : public party
{
public:
	npc_party(wml::const_node_ptr node, world& gameworld);

	void friendly_encounter(party& p);

private:
	bool is_human_controlled() const { return false; }
	TURN_RESULT do_turn();
	wml::const_node_ptr dialog_;

	hex::location current_destination_;
	std::vector<hex::location> wander_between_;
};
		
}

#endif
