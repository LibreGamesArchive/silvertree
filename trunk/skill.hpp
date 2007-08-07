
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SKILL_HPP_INCLUDED
#define SKILL_HPP_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "battle_move_fwd.hpp"
#include "character_fwd.hpp"
#include "formula.hpp"
#include "skill_fwd.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class skill_effect;
typedef boost::shared_ptr<skill_effect> skill_effect_ptr;
typedef boost::shared_ptr<const skill_effect> const_skill_effect_ptr;

class skill_requirement;
typedef boost::shared_ptr<skill_requirement> skill_requirement_ptr;
typedef boost::shared_ptr<const skill_requirement> const_skill_requirement_ptr;

class skill {
public:
	static const_skill_ptr get_skill(const std::string& name);
	static void add_skill(wml::const_node_ptr node);

	typedef std::map<std::string,const_skill_ptr> skills_map;
	static const skills_map& all_skills();

	explicit skill(const wml::const_node_ptr& node);

	const std::string& name() const { return name_; }
	const std::string& description() const { return description_; }
	const std::string& prerequisite() const { return prerequisite_; }

	int effect_on_stat(const character& c, const std::string& stat) const;
	const const_battle_move_ptr& move() const {
		return move_;
	}

	bool is_active(const character& c) const;
	int cost(const character& c) const;

private:
	std::string name_;
	std::string description_;
	std::string prerequisite_;
	std::vector<skill_requirement_ptr> requirements_;
	std::map<std::string,const_formula_ptr> effects_;
	const_battle_move_ptr move_;
	formula cost_;
};

}

#endif
