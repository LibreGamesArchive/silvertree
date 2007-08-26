
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "equipment.hpp"
#include "map_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

#include <boost/lexical_cast.hpp>
#include <set>

namespace game_logic
{

namespace {
const std::set<std::string>& item_stat() {
	static std::set<std::string> stats;
	if(stats.empty()) {
		stats.insert("value");
	}

	return stats;
}
}

equipment::equipment(ITEM_TYPE type, const wml::const_node_ptr& node)
  : item(type, node), damage_type_(wml::get_str(node,"damage_type"))
{
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		if(item_stat().count(i->first)) {
			continue;
		}

		try {
			const int res = boost::lexical_cast<int>(i->second);
			stats_[i->first] = res;
		} catch(boost::bad_lexical_cast& e) {
			//not an error -- anything that can't be converted to
			//a number isn't a stat.
		}
	}

	std::vector<wml::const_node_ptr> parries = wml::child_nodes(node, "parry_against");
	foreach(const wml::const_node_ptr& parry, parries) {
		for(wml::node::const_attr_iterator i = parry->begin_attr(); i != parry->end_attr(); ++i) {
			parry_against_[i->first] = boost::lexical_cast<int>(i->second);
		}
	}
}

equipment::equipment(ITEM_TYPE type) : item(type)
{}

int equipment::modify_stat(const std::string& stat, bool *present) const
{
	bool dummy;
	if(!present) {
		present = &dummy;
	}
	const std::map<std::string,int>::const_iterator i = stats_.find(stat);
	if(i == stats_.end()) {
		*present = false;
		return 0;
	} else {
		*present = true;
		return i->second;
	}
}

int equipment::parry_against(const std::string& damage_type) const
{
	return map_get_value_default(parry_against_,damage_type,100);
}

}
