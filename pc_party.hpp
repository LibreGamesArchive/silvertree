
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef PC_PARTY_HPP_INCLUDED
#define PC_PARTY_HPP_INCLUDED

#include "party.hpp"
#include "wml_node.hpp"

namespace game_logic
{

class world;

class pc_party : public party
{
public:
	pc_party(wml::const_node_ptr node, world& game_world);

	bool is_human_controlled() const { return true; }

	void friendly_encounter(party& p);
private:
	TURN_RESULT do_turn();
		
};
		
}

#endif
