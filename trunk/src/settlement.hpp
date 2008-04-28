
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SETTLEMENT_HPP_INCLUDED
#define SETTLEMENT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "formula_fwd.hpp"
#include "model_fwd.hpp"
#include "party_fwd.hpp"
#include "settlement_fwd.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

namespace hex
{
	class gamemap;
}

namespace game_logic
{

class settlement
{
public:
	settlement(const wml::const_node_ptr& node,
	           const hex::gamemap& map);

	wml::node_ptr write() const;

	void entry_points(std::vector<hex::location>& result) const;
	bool has_entry_point(const hex::location& loc) const;
	void draw() const;
	game_time enter(party_ptr pty, const hex::location& loc,
					const game_time& t);
	void play();
	const world& get_world() const;
private:
	std::map<hex::location, hex::location> portals_;
	graphics::const_model_ptr model_;
	const_formula_ptr model_height_formula_;
	const_formula_ptr model_rotation_formula_;
	wml::const_node_ptr wml_;
	mutable boost::shared_ptr<world> world_;
	const hex::gamemap& map_;
};

}

#endif