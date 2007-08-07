
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef EQUIPMENT_HPP_INCLUDED
#define EQUIPMENT_HPP_INCLUDED

#include <map>
#include <vector>

#include "item.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class equipment : public item
{
public:
	equipment(ITEM_TYPE type, const wml::const_node_ptr& node);
	explicit equipment(ITEM_TYPE type);

	int modify_stat(const std::string& stat, bool* present=NULL) const;
	const std::map<std::string,int>& stats() const { return stats_; }
	const std::string& damage_type() const { return damage_type_; }
	int parry_against(const std::string& damage_type) const;
private:
	std::map<std::string,int> stats_;
	std::map<std::string,int> parry_against_;
	std::string damage_type_;
};

}

#endif
