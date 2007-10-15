
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
#include "character_generator.hpp"
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
	variant get_value(const std::string& key) const {
		if(key == "shortfall") {
			return val_;
		} else {
			return variant();
		}
	}
	void get_inputs(std::vector<formula_input>* inputs) const {
		inputs->push_back(formula_input("shortfall", FORMULA_READ_ONLY));
	}
public:
	explicit strength_penalty_callable(int val) : val_(val) {}
};

class fatigue_penalty_callable : public formula_callable
{
	const character& char_;
	variant get_value(const std::string& stat) const { return variant(char_.stat_before_fatigue(stat)); }
	void get_inputs(std::vector<formula_input>* inputs) const {
		foreach(const std::string& str, character::attributes()) {
			inputs->push_back(formula_input(str, FORMULA_READ_ONLY));
		}
	}
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

wml::node_ptr character::write() const
{
	wml::node_ptr res(new wml::node("character"));
	WML_WRITE_ATTR(res, description);
	WML_WRITE_ATTR(res, fatigue);
	WML_WRITE_ATTR(res, level);
	WML_WRITE_ATTR(res, xp);
	WML_WRITE_ATTR(res, hitpoints);
	WML_WRITE_ATTR(res, image);
	WML_WRITE_ATTR(res, model);
	WML_WRITE_ATTR(res, portrait);
	WML_WRITE_ATTR(res, bar_portrait);
	WML_WRITE_ATTR(res, improvement_points);
	WML_WRITE_ATTR(res, spent_skill_points);

	switch(alignment_) {
	case LAWFUL:
		res->set_attr("alignment", "lawful");
		break;
	case NEUTRAL:
		res->set_attr("alignment", "neutral");
		break;
	case CHAOTIC:
		res->set_attr("alignment", "chaotic");
		break;
	default:
		assert(false);
	}

	res->add_child(wml::write_attribute_map("movement_costs", move_cost_map_));
	res->add_child(wml::write_attribute_map("attributes", attributes_));

	std::string equipment;
	foreach(const_item_ptr equip, equipment_) {
		if(!equipment.empty()) {
			equipment += ",";
		}

		equipment += equip->id();
	}

	res->set_attr("equipment", equipment);

	std::string skills;
	foreach(const_skill_ptr s, skills_) {
		if(!skills.empty()) {
			skills += ",";
		}

		skills += s->name();
	}

	res->set_attr("skills", skills);
	res->set_attr("color", color_.str());

	return res;
}

character::character(wml::const_node_ptr node)
  : color_(wml::get_str(node, "color", "rgb(100,100,100)"))
{
	character_generator::get((*node)["id"]).generate(*this, node);
}

const std::string& character::portrait(bool for_bar) const
{
	if(for_bar && !bar_portrait_.empty()) {
		return bar_portrait_;
	}
	return portrait_;
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
	}

	if(base_cost < 0) {
		return -1;
	}

	if(feature) {
		int feature_cost = feature->default_cost();
		const std::map<std::string,int>::const_iterator itor =
		      move_cost_map_.find(feature->name());
		if(itor != move_cost_map_.end()) {
			feature_cost = itor->second;
		}

		if(feature_cost < 0) {
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
const std::string StaminaStat = "stamina";
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
		if(p.second->cost(*this) <= points &&
		   (prerequisite.empty() || has_skill(prerequisite))) {
			res.push_back(p.second);
		}
	}

	return res;
}

void character::get_inputs(std::vector<formula_input>* inputs) const
{
	inputs->push_back(formula_input("level", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("weapon", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("description", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("max_hp", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("max_hitpoints", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("hp", FORMULA_READ_WRITE));
	inputs->push_back(formula_input("hitpoints", FORMULA_READ_WRITE));
	inputs->push_back(formula_input("strength", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("agility", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("endurance", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("persona", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("intelligence", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("persona", FORMULA_READ_ONLY));
}

variant character::get_value(const std::string& key) const
{
	std::map<std::string,int>::const_iterator i = attributes_.find(key);
	if(i != attributes_.end()) {
		return variant(i->second);
	}

	static const std::string level_str = "level";
	static const std::string weapon_str = "weapon";
	if(key == level_str) {
		return variant(level());
	} else if(key == weapon_str) {
		if(const game_logic::equipment* equip = weapon()) {
			return variant(equip);
		} else {
			static game_logic::equipment empty_weapon(EQUIPMENT_WEAPON);
			return variant(&empty_weapon);
		}
	} else if(key == "description") {
		return variant(description_);
	} else if(key == "max_hp" || key == "max_hitpoints") {
		std::cerr << "get max hp: " << max_hitpoints() << "\n";
		return variant(max_hitpoints());
	} else if(key == "hp" || key == "hitpoints") {
		return variant(hitpoints());
	}

	return variant(base_stat(key));
}

void character::set_value(const std::string& key, const variant& value)
{
	if(key == "hitpoints" || key == "hp") {
		hitpoints_ = value.as_int();
		std::cerr << "modify hitpoints to: " << hitpoints_ << "\n";
	} else {
		formula_callable::set_value(key, value);
	}
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
		if(s->is_active(*this)) {
			foreach(const const_battle_move_ptr& move, s->moves()) {
				moves_.push_back(move);
			}
		}
	}
}

bool character::can_improve_attr(const std::string& str) const
{
	return improvement_cost(get_attr(str)) <= improvement_points();
}

void character::improve_attr(const std::string& str)
{
	const int before_max_hp = max_hitpoints();
	improvement_points_ -= improvement_cost(get_attr(str));
	attributes_[str]++;
	const int hp_increase = max_hitpoints() - before_max_hp;
	hitpoints_ += hp_increase;
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
				if(shortfall > 0) {
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
	return std::max<int>(1,stat(MaxHitpointsStat));
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

int character::defense_behind() const
{
	return dodge();
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

int character::stamina() const
{
	return stat(StaminaStat);
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
		const int old_maxhp = max_hitpoints();
		++level_;
		hitpoints_ += max_hitpoints() - old_maxhp;
		improvement_points_ += formula_registry::get_stat_calculation("improvement_points")->execute(*this).as_int();
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

bool character::can_attack_engaged() const
{
	const game_logic::equipment* w = weapon();
	return w == NULL || w->can_attack_engaged();
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

variant character::color() const
{
	return color_.execute(*this);
}

}
