
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BATTLE_MODIFICATION_HPP_INCLUDED
#define BATTLE_MODIFICATION_HPP_INCLUDED

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "battle_character_fwd.hpp"
#include "formula_fwd.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class battle_modification
{
public:
	explicit battle_modification(wml::const_node_ptr node);
	void apply(battle_character& src, battle_character& target, int current_time) const;
	enum TARGET_TYPE { TARGET_SELF, TARGET_ENEMY, TARGET_FRIEND, TARGET_ALL };
	TARGET_TYPE target() const { return target_; }
	int range() const;
	int radius() const;
private:
	std::map<std::string,formula_ptr> mods_;
	formula_ptr duration_;
	formula_ptr range_;
	formula_ptr radius_;
	TARGET_TYPE target_;
};

typedef boost::shared_ptr<battle_modification> battle_modification_ptr;
typedef boost::shared_ptr<const battle_modification> const_battle_modification_ptr;

}

#endif
