
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <map>

#include "battle_move.hpp"
#include "character.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "item.hpp"
#include "skill.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace game_logic {

class skill_requirement {
public:
	explicit skill_requirement(const wml::const_node_ptr& node);
	bool met(const character& c) const;
private:
	std::string item_class_;
};

skill_requirement::skill_requirement(const wml::const_node_ptr& node)
  : item_class_(wml::get_str(node,"item_class"))
{
}

bool skill_requirement::met(const character& c) const
{
	if(!item_class_.empty()) {
		bool found = false;
		foreach(const item_ptr& i, c.equipment()) {
			if(i->item_class() == item_class_) {
				found = true;
				break;
			}
		}

		if(!found) {
			return false;
		}
	}

	return true;
}

namespace {
skill::skills_map skills_registry;
}

const_skill_ptr skill::get_skill(const std::string& name)
{
	const skills_map::const_iterator i =
	       skills_registry.find(name);
	if(i != skills_registry.end()) {
		return i->second;
	} else {
		return const_skill_ptr();
	}
}

void skill::add_skill(wml::const_node_ptr node)
{
	const_skill_ptr new_skill(new skill(node));
	skills_registry.insert(std::pair<std::string,const_skill_ptr>(new_skill->name(),new_skill));
}

const skill::skills_map& skill::all_skills()
{
	return skills_registry;
}

skill::skill(const wml::const_node_ptr& node)
  : name_(wml::get_str(node,"name")),
    description_(wml::get_str(node,"description")),
    cost_(wml::get_str(node,"cost")),
    prerequisite_(wml::get_str(node,"prerequisite"))
{
	WML_READ_VECTOR(node, requirements_, new skill_requirement, "requirement", skill_requirement_ptr);

	wml::const_node_ptr effects = node->get_child("effects");
	if(effects) {
		for(wml::node::const_attr_iterator i = effects->begin_attr();
		    i != effects->end_attr(); ++i) {
			effects_[i->first] = const_formula_ptr(new formula(i->second));
		}
	}

	wml::const_node_ptr move = node->get_child("move");
	if(move) {
		move_.reset(new battle_move(move));
	}
}

int skill::effect_on_stat(const character& c, const std::string& stat) const
{
	if(!is_active(c)) {
		return 0;
	}

	std::map<std::string,const_formula_ptr>::const_iterator f = effects_.find(stat);
	if(f == effects_.end()) {
		return 0;
	}

	return f->second->execute(c);
}

bool skill::is_active(const character& c) const
{
	foreach(const skill_requirement_ptr& r, requirements_) {
		if(!r->met(c)) {
			return false;
		}
	}

	return true;
}

int skill::cost(const character& c) const
{
	return cost_.execute(c);
}

}
