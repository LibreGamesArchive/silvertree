
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef CHARACTER_HPP_INCLUDED
#define CHARACTER_HPP_INCLUDED

#include <map>
#include <string>
#include <vector>

#include "base_terrain_fwd.hpp"
#include "battle_move_fwd.hpp"
#include "character_fwd.hpp"
#include "formula.hpp"
#include "item_fwd.hpp"
#include "skill_fwd.hpp"
#include "terrain_feature_fwd.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class character : public game_logic::formula_callable
{
public:
	friend class character_generator;
	static const std::vector<std::string>& attributes();

	static character_ptr create(wml::const_node_ptr node);

	int move_cost(hex::const_base_terrain_ptr terrain,
	              hex::const_terrain_feature_ptr feature,
	              int height_change) const;
	int vision_cost(hex::const_base_terrain_ptr terrain) const;
	int vision() const;
	const std::string& description() const { return description_; }
	int initiative() const;
	int hitpoints() const { return hitpoints_; }
	int max_hitpoints() const;
	int speed() const;
	int climbing() const;
	int experience() const { return xp_; }
	int experience_required() const;
	int damage() const;
	const std::string& damage_type() const;
	int attack() const;
	int defense(const std::string& damage_type="") const;
	int parry(const std::string& damage_type="") const;
	int dodge() const;
	
	//the character's resistance: on each blow, will ignore the first
	//percent% of the first 'amount' of damage.
	void get_resistance(const std::string& damage_type,
	                    int* amount, int* percent);
	bool take_damage(int amount);
	bool dead() const;
	void set_to_near_death() { hitpoints_ = 1; }

	void use_stamina(int amount) {
		fatigue_ += amount;
		if(fatigue_ < 0) { fatigue_ = 0; }
	}

	const std::string& image() const { return image_; }
	const std::string& portrait() const { return portrait_; }

	enum ALIGNMENT { CHAOTIC = -1, NEUTRAL, LAWFUL };
	ALIGNMENT alignment() const { return alignment_; }
	std::string alignment_description() const;

	int fatigue() const { return fatigue_; }
	int stamina() const;

	bool award_experience(int xp);
	int level() const { return level_; }

	void full_heal() { hitpoints_ = max_hitpoints(); fatigue_ = 0; }

	int get_attr(const std::string& str) const;
	bool can_improve_attr(const std::string& str) const;
	void improve_attr(const std::string& str);
	void learn_skill(const std::string& name);
	bool has_skill(const std::string& name) const;

	int base_stat(const std::string& str) const;
	int stat(const std::string& str) const;
	int stat_before_fatigue(const std::string& str) const;
	int get_equipment_mod(const std::string& str) const;
	int get_skill_mod(const std::string& str) const;
	int stat_mod_height(const std::string& str, int height_diff) const;

	const std::vector<item_ptr>& equipment() const { return equipment_; }
	int attack_range() const;
	const game_logic::equipment* weapon() const;
	void swap_equipment(int index, item_ptr& new_item);

	int improvement_points() const { return improvement_points_; }
	int skill_points() const { return total_skill_points() - spent_skill_points_; }

	const std::vector<const_skill_ptr>& skills() const { return skills_; }
	std::vector<const_skill_ptr> eligible_skills() const;

	const std::vector<const_battle_move_ptr>& battle_moves() const {
		return moves_;
	}

	class final_stat_callable : public game_logic::formula_callable {
	public:
		explicit final_stat_callable(const character& c) : char_(c)
		{}

	private:
		variant get_value(const std::string& key) const {
			int res = char_.stat(key);
			if(res == 0) {
				res = char_.get_attr(key);
			}
			return variant(res);
		}
		const character& char_;
	};

private:
	explicit character(wml::const_node_ptr node);

	variant get_value(const std::string& key) const;
	int total_skill_points() const;

	void calculate_moves();

	std::string description_;
	int hitpoints_;
	int fatigue_;
	int level_;
	int xp_;

	std::map<std::string,int> move_cost_map_;
	std::map<std::string,int> attributes_;

	std::string image_;
	std::string portrait_;
	ALIGNMENT alignment_;

	std::vector<item_ptr> equipment_;
	std::vector<const_skill_ptr> skills_;
	std::vector<const_battle_move_ptr> moves_;

	int improvement_points_;
	int spent_skill_points_;
};
		
}

#endif
