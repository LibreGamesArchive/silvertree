
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
#include "map_avatar.hpp"
#include "model_fwd.hpp"
#include "party_fwd.hpp"
#include "settlement_fwd.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"
#include "world.hpp"

namespace hex
{
	class gamemap;
}

namespace game_logic
{

class settlement: public hex::basic_drawable
{
public:
	settlement(const wml::const_node_ptr& node,
	           const hex::gamemap& map);

	wml::node_ptr write() const;

	void entry_points(std::vector<hex::location>& result) const;
	bool has_entry_point(const hex::location& loc) const;
	game_time enter(party_ptr pty, const hex::location& loc,
					const game_time& t, const world& from);
	void play();
	const world& get_world() const;
    const std::vector<hex::const_map_avatar_ptr>& avatars() { return avatars_; }

    void update_rotation(int key) const;
    void update_position(int key) const;
private:
	std::map<hex::location, hex::location> portals_;
    std::map<int, hex::location> avatar_keys_;
    std::vector<hex::const_map_avatar_ptr> avatars_;
	const_formula_ptr model_height_formula_;
	const_formula_ptr model_rotation_formula_;
	wml::const_node_ptr wml_;
	mutable boost::shared_ptr<world> world_;
	const hex::gamemap& map_;
};

typedef boost::shared_ptr<const settlement> const_settlement_ptr;

}

#endif
