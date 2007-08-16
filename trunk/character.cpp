
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "base_terrain.hpp"
#include "battle_move.hpp"
#include "character.hpp"
#include "equipment.hpp"
#include "foreach.hpp"
#include "formula_registry.hpp"
#include "item.hpp"
#include "skill.hpp"
#include "string_utils.hpp"
#include "terrain_feature.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

#include <assert.h>
#include <iostream>

namespace game_logic
{

namespace {

const int MaxClimb = 200;
const int DownHillFree = 50;

class strength_penalty_callable : public formula_callable
{
	variant val_;
	variant get_value(const std::string& key) const { return val_; }
public:
	explicit strength_penalty_callable(int val) : val_(val) {}
};

class fatigue_penalty_callable : public formula_callable
{
	const character& char_;
	variant get_value(const std::string& stat) const { return variant(char_.stat_before_fatigue(stat)); }
public:
	explicit fatigue_penalty_callable(const character& c) : char_(c)
	{}
};
		
}

const std::vector<std::string>& character::attributes()
{
	static std::vector<std::string> res;
	if(res.empty()) {
		const std::string attr[] = {
		  "strength", "agility", "endurance",
		  "intelligence", "perception", "persona"};
		foreach(const std::string& a, attr) {
			res.push_back(a);
		}
	}

	return res;
}

character_ptr character::create(wml::const_node_ptr node)
{
	return character_ptr(new character(node));
}

character::character(wml::const_node_ptr node)
   : description_(wml::get_attr<std::string>(node,"description")),
	 image_(wml::get_str(node,"image")),
	 portrait_(wml::get_str(node,"portrait")),
	 hitpoints_(wml::get_attr<int>(node,"hitpoints",-1)),
	 fatigue_(wml::get_attr<int>(node,"fatigue")),
	 level_(wml::get_attr<int>(node,"level",1)),
	 xp_(wml::get_attr<int>(node,"xp")),
	 level_up_(node->get_child("level_up")),
	 alignment_(character::NEUTRAL),
	 improvement_points_(wml::get_int(node,"improvements")),
	 spent_skill_points_(wml::get_int(node,"spent_skill_points"))
{
	wml::const_node_ptr costs = node->get_child("movement_costs");
	if(costs) {
		for(wml::node::const_attr_iterator i = costs->begin_attr();
		    i != costs->end_attr(); ++i) {
			move_cost_map_[i->first] =
			         wml::get_attr<int>(costs,i->first,-1);
		}
	}

	wml::const_node_ptr attr = node->get_child("attributes");
	if(attr) {
		for(wml::node::const_attr_iterator i = attr->begin_attr();
		    i != attr->end_attr(); ++i) {
			attributes_[i->first] = wml::get_attr<int>(attr,i->first);
		}
	}

	wml::node::const_child_range equip = node->get_child_range("equipment");
	while(equip.first != equip.second) {
		const wml::const_node_ptr eq = equip.first->second;
		const item_ptr item = item::create_item(eq);
		if(item) {
			equipment_.push_back(item);
		}

		++equip.first;
	}

	const std::vector<std::string> skills = util::split(wml::get_str(node,"skills"));
	foreach(const std::string& skill, skills) {
		skills_.push_back(skill::get_skill(skill));
	}

	const std::string& align = wml::get_attr<std::string>(node,"alignment");
	if(align == "chaotic") {
		alignment_ = CHAOTIC;
	} else if(align == "lawful") {
		alignment_ = LAWFUL;
	}

	if(hitpoints_ == -1) {
		hitpoints_ = max_hitpoints();
	}

	calculate_moves();
}

int character::move_cost(hex::const_base_terrain_ptr terrain,
                         hex::const_terrain_feature_ptr feature,
                         int height_change) const
{
	int base_cost = terrain->default_cost();
	const std::map<std::string,int>::const_iterator itor =
	        move_cost_map_.find(terrain->name());
	if(itor != move_cost_map_.end()) {
		base_cost = itor->second;
		if(base_cost == -1) {
			return -1;
		}
	}

	if(feature) {
		int feature_cost = feature->default_cost();
		const std::map<std::string,int>::const_iterator itor =
		      move_cost_map_.find(feature->name());
		if(itor != move_cost_map_.end()) {
			feature_cost = itor->second;
		}

		if(feature_cost == -1) {
			return -1;
		}

		base_cost += feature_cost;
	}

	if(abs(height_change) > MaxClimb) {
		return -1;
	}

	if(height_change < 0) {
		height_change += DownHillFree;
		if(height_change > 0) {
			height_change = 0;
		}
	}

	height_change = abs(height_change);
	const int climb_cost = 100 + (height_change*100*2)/climbing();

	return (climb_cost*base_cost)/(speed()*100);
}

namespace {
const int DefaultAttribute = 10;
const std::string StrengthAttribute = "strength";
const std::string AgilityAttribute = "agility";
const std::string EnduranceAttribute = "endurance";
const std::string IntelligenceAttribute = "intelligence";
const std::string PerceptionAttribute = "perception";
const std::string PersonaAttribute = "persona";

const std::string MaxHitpointsStat = "max_hitpoints";
const std::string InitiativeStat = "initiative";
const std::string AttackStat = "attack";
const std::string DamageStat = "damage";
const std::string DodgeStat = "dodge";
const std::string ParryStat = "parry";
const std::string SpeedStat = "speed";
const std::string ClimbStat = "climb";
const std::string VisionStat = "vision";
const std::string EnergyStat = "energy";
const std::string ResistanceStat = "resistance";
const std::string ResistancePercentStat = "resistance_percent";
const std::string SkillPointsStat = "skill_points";

int improvement_cost(int attr)
{
	if(attr < 12) {
		return 1;
	} else {
		return 1 + (attr-10)/2;
	}
}

}

int character::get_attr(const std::string& str) const
{
	const std::map<std::string,int>::const_iterator i =
	       attributes_.find(str);
	if(i != attributes_.end()) {
		return i->second;
	} else {
		return DefaultAttribute;
	}
}

std::vector<const_skill_ptr> character::eligible_skills() const
{
	std::vector<const_skill_ptr> res;
	const skill::skills_map& skills = skill::all_skills();
	const int points = skill_points();
	typedef std::pair<std::string,const_skill_ptr> value;
	foreach(const value& p, skills) {
		if(has_skill(p.first)) {
			continue;
		}

		const std::string& prerequisite = p.second->prerequisite();
		if(p.second->cost(*this) < points &&
		   (prerequisite.empty() || has_skill(prerequisite))) {
			res.push_back(p.second);
		}
	}

	return res;
}

variant character::get_value(const std::string& key) const
{
	std::map<std::string,int>::const_iterator i = attributes_.find(key);
	if(i != attributes_.end()) {
		return variant(i->second);
	}

	static const std::string level_str = "level";
	if(key == level_str) {
		return variant(level());
	}

	return variant(base_stat(key));
}

int character::total_skill_points() const
{
	return stat(SkillPointsStat);
}

void character::calculate_moves()
{
	moves_.clear();
	moves_.push_back(battle_move::standard_move());
	moves_.push_back(battle_move::standard_attack());
	moves_.push_back(battle_move::standard_pass());
	foreach(const const_skill_ptr& s, skills_) {
		if(s->is_active(*this) && s->move()) {
			moves_.push_back(s->move());
		}
	}
}

bool character::can_improve_attr(const std::string& str) const
{
	return improvement_cost(get_attr(str)) <= improvement_points();
}

void character::improve_attr(const std::string& str)
{
	improvement_points_ -= improvement_cost(get_attr(str));
	attributes_[str]++;
}

void character::learn_skill(const std::string& name)
{
	const const_skill_ptr s = skill::get_skill(name);
	if(!s) {
		std::cerr << "could not find skill '" << name << "'\n";
		return;
	}

	spent_skill_points_ += s->cost(*this);
	skills_.push_back(s);
	calculate_moves();
}

bool character::has_skill(const std::string& name) const
{
	foreach(const const_skill_ptr& s, skills_) {
		if(s->name() == name) {
			return true;
		}
	}

	return false;
}

int character::base_stat(const std::string& str) const
{
	return formula::evaluate(formula_registry::get_stat_calculation(str),*this).as_int();
}

int character::stat(const std::string& str) const
{
	const const_formula_ptr& penalty =
	   formula_registry::get_fatigue_penalty(str);
	if(penalty) {
		return penalty->execute(fatigue_penalty_callable(*this)).as_int();
	} else {
		return stat_before_fatigue(str);
	}
}

int character::stat_before_fatigue(const std::string& str) const
{
	if(str == "fatigue") {
		return fatigue();
	}

	return base_stat(str) + get_equipment_mod(str) + get_skill_mod(str);
}

int character::get_equipment_mod(const std::string& str) const
{
	const const_formula_ptr strength_penalty = formula_registry::get_strength_penalty(str);
	int res = 0;
	foreach(const item_ptr& item, equipment_) {
		const game_logic::equipment* equip = item_as_equipment(item);
		if(equip) {
			res += equip->modify_stat(str);

			if(strength_penalty) {
				static const std::string IdealStrength = "ideal_strength";
				const int shortfall = equip->modify_stat(IdealStrength) - get_attr(StrengthAttribute);
				if(shortfall < 0) {
					strength_penalty_callable callable(shortfall);
					res += strength_penalty->execute(callable).as_int();
				}
			}
		}
	}

	return res;
}

int character::get_skill_mod(const std::string& str) const
{
	int res = 0;
	foreach(const const_skill_ptr& skill, skills_) {
		res += skill->effect_on_stat(*this, str);
	}

	return res;
}

int character::stat_mod_height(const std::string& str, int height_diff) const
{
	return formula::evaluate(formula_registry::get_height_advantage(str),
	            map_formula_callable(this).add("height",
						                       variant(height_diff))).as_int();
}

int character::vision_cost(hex::const_base_terrain_ptr terrain) const
{
	return 100;
}

int character::initiative() const
{
	return stat(InitiativeStat);
}

int character::max_hitpoints() const
{
	return stat(MaxHitpointsStat);
}

int character::speed() const
{
	return stat(SpeedStat);
}

int character::climbing() const
{
	return stat(ClimbStat);
}

int character::vision() const
{
	return stat(VisionStat);
}

int character::experience_required() const
{
	return stat("experience_required");
}

int character::damage() const
{
	return stat(DamageStat);
}

const std::string& character::damage_type() const
{
	const game_logic::equipment* equip = weapon();
	if(equip) {
		return equip->damage_type();
	} else {
		static std::string empty;
		return empty;
	}
}

int character::attack() const
{
	return stat(AttackStat);
}

int character::defense(const std::string& damage_type) const
{
	return std::max(dodge(),parry(damage_type));
}

int character::dodge() const
{
	return stat(DodgeStat);
}

int character::parry(const std::string& damage_type) const
{
	const int base = base_stat(ParryStat) + get_skill_mod(ParryStat);
	int best = 0;
	foreach(const item_ptr& item, equipment_) {
		const game_logic::equipment* equip = item_as_equipment(item);
		if(equip) {
			const int equip_parry = equip->modify_stat(ParryStat);
			if(equip_parry > 0) {
				const int res = ((base + equip_parry) * equip->parry_against(damage_type))/100;
				if(res > best) {
					best = res;
				}
			}
		}
	}

	return best;
}

void character::get_resistance(const std::string& damage_type,
                               int* amount, int* percent)
{
	*amount = 0;
	*percent = 0;

	foreach(const item_ptr& item, equipment_) {
		const game_logic::equipment* equip = item_as_equipment(item);
		*amount += equip->modify_stat(ResistanceStat);
		bool present;
		int perc = equip->modify_stat(damage_type,&present);
		if(!present) {
			perc = equip->modify_stat(ResistancePercentStat);
		}

		*percent += perc;
	}
}

int character::energy() const
{
	return stat(EnergyStat);
}

bool character::take_damage(int amount)
{
	hitpoints_ -= amount;
	if(hitpoints_ <= 0) {
		return true;
	} else if(hitpoints_ > max_hitpoints()) {
		hitpoints_ = max_hitpoints();
	}

	return false;
}

bool character::dead() const
{
	return hitpoints_ <= 0;
}

std::string character::alignment_description() const
{
	switch(alignment_) {
	case LAWFUL: return "Lawful";
	case NEUTRAL: return "Neutral";
	case CHAOTIC: return "Chaotic";
	default:
		assert(false);
	}
}

bool character::award_experience(int xp)
{
	xp_ += xp;
	if(xp_ >= experience_required()) {
		++level_;
		return true;
	} else {
		return false;
	}
}

int character::attack_range() const
{
	static const std::string RangeStat = "range";
	const game_logic::equipment* w = weapon();
	if(w) {
		return w->modify_stat(RangeStat);
	} else {
		return 0;
	}
}

const equipment* character::weapon() const
{
	foreach(const item_ptr& i, equipment_) {
		if(i->type() == EQUIPMENT_WEAPON) {
			const game_logic::equipment* equip = item_as_equipment(i);
			return equip;
		}
	}

	return NULL;
}

void character::swap_equipment(int index, item_ptr& new_item)
{
	assert(index >= 0 && index < equipment_.size());
	assert(new_item->type() == equipment_[index]->type());
	equipment_[index].swap(new_item);
	calculate_moves();
}

}
