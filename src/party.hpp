
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef PARTY_HPP_INCLUDED
#define PARTY_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <deque>
#include <set>
#include <vector>

#include "character_fwd.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "gamemap.hpp"
#include "game_time.hpp"
#include "item_fwd.hpp"
#include "map_avatar.hpp"
#include "party_fwd.hpp"
#include "pathfind.hpp"
#include "tile_logic.hpp"
#include "wml_node.hpp"

namespace game_logic
{

class world;

class party : public formula_callable, public hex::path_cost_calculator
{
public:
	party(wml::const_node_ptr node, world& gameworld);
	virtual ~party();

	virtual wml::node_ptr write() const;
	virtual void set_destination(const hex::location& loc) {}
	virtual const std::vector<hex::location>* get_current_path() const {
		return NULL;
	}

	static party_ptr create_party(
					wml::const_node_ptr node,
					world& game_world);

	int id() const { return id_; }

	void draw();
	hex::DIRECTION last_move() const { return last_move_; }
	const hex::location& previous_loc() const { return previous_loc_; }
	const hex::location& loc() const { return loc_; }
	void set_loc(const hex::location& loc) { loc_ = loc; }

	void new_world(world& w, const hex::location& loc, hex::DIRECTION dir=hex::NULL_DIRECTION);

	enum TURN_RESULT { TURN_COMPLETE, TURN_STILL_THINKING };
	TURN_RESULT play_turn();

	virtual bool is_human_controlled() const = 0;
	bool is_enemy(const party& p) const;

	game_time ready_to_move_at() const;

	void get_pos(GLfloat* pos) const;
	GLfloat get_rotation() const;

	const std::set<hex::location>& get_visible_locs() const;
	void get_visible_parties(std::vector<const_party_ptr>& parties) const;

	int vision() const;
	int track() const;
	int heal() const;
	int haggle() const;

	int trackability() const;

	std::vector<character_ptr>::const_iterator begin_members() const { return members_.begin(); }
	std::vector<character_ptr>::const_iterator end_members() const { return members_.end(); }
	const std::vector<character_ptr>& members() { return members_; }
	void destroy();
	bool is_destroyed() const;

	std::string status_text() const;
	std::string fatigue_status_text() const;

	const std::string& allegiance() const { return allegiance_; }

	virtual void encounter(party& p, const std::string& type) {}

	world& game_world() { return *world_; }
	const world& game_world() const { return *world_; }

	enum MOVEMENT_MODE { WALK = 1, RUN };
	void set_movement_mode(MOVEMENT_MODE mode) { move_mode_ = mode; }

	void full_heal();

	void join_party(character_ptr new_char);
	void merge_party(party& joining_party);

	void acquire_item(item_ptr i);

	const std::vector<item_ptr>& inventory() const {
		return inventory_;
	}

	void assign_equipment(character_ptr c,
	                      int char_item, int party_item);

	int money() const { return money_; }
	void get_money(int m) { money_ += m; }

	bool has_script() const { return !scripted_moves_.empty(); }
	void add_scripted_move(const hex::location& loc) {
		scripted_moves_.push_back(loc);
	}

	void pass(int minutes=1);
	void finish_move();

protected:
	void move(hex::DIRECTION dir);
	int movement_cost(const hex::location& src,
	                  const hex::location& dst) const;
	bool allowed_to_move(const hex::location& loc) const;
	const hex::gamemap& map() const;
	virtual void set_value(const std::string& key, const variant& value);
	virtual void enter_new_world(const world& w) {}

private:
	int aggregate_stat_max(const std::string& stat) const;

	// function which aggregates a party stat based on the
	// "expert and assistant method". This method has the assumption that
	// the skill is primarily performed by an 'expert' -- the best character
	// in the party at this stat. The expert is assisted by an 'assistant' --
	// the second best character in the party at this stat. Further
	// characters in the party do not impact the final rating (too many cooks
	// spoil the broth).
	//
	// The formula is expert + assistant/2
	int aggregate_stat_expert_and_assistant(const std::string& stat) const;

	void apply_fatigue(const hex::location& src,
	                   const hex::location& dst);

	virtual TURN_RESULT do_turn() = 0;

	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<formula_input>* inputs) const;

	int id_;
	std::string str_id_;

	world* world_;
	hex::map_avatar_ptr avatar_;
	hex::location loc_, previous_loc_;
	hex::DIRECTION facing_, last_facing_;
	hex::DIRECTION last_move_;
	game_time departed_at_, arrive_at_;

	std::vector<character_ptr> members_;

	std::string allegiance_;

	mutable std::set<hex::location> visible_locs_;

	MOVEMENT_MODE move_mode_;

	std::vector<item_ptr> inventory_;
	int money_;

	std::deque<hex::location> scripted_moves_;
};

}

#endif
