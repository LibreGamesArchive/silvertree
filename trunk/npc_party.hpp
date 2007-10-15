
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

#include "formula_fwd.hpp"
#include "party.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class npc_party : public party
{
public:
	npc_party(wml::const_node_ptr node, world& gameworld);

	void encounter(party& p, const std::string& type);

	wml::node_ptr write() const;

private:
	void set_value(const std::string& key, const variant& value);
	bool is_human_controlled() const { return false; }
	TURN_RESULT do_turn();
	void choose_new_destination();
	wml::const_node_ptr dialog_;

	hex::location current_destination_;
	bool aggressive_;
	bool rest_;
	std::vector<hex::location> wander_between_;
	const_formula_ptr next_destination_;
};

}

#endif
