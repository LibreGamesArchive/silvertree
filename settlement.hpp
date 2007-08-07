
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

	const hex::location& loc() const { return loc_; }
	void draw() const;
	void enter(party_ptr pty);
private:
	hex::location loc_;
	graphics::const_model_ptr model_;
	wml::const_node_ptr wml_;
	boost::shared_ptr<world> world_;
	const hex::gamemap& map_;
};
		
}

#endif
