
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
#include <set>
#include <vector>

#include "character_fwd.hpp"
#include "gamemap.hpp"
#include "game_time.hpp"
#include "item_fwd.hpp"
#include "map_avatar.hpp"
#include "party_fwd.hpp"
#include "tile_logic.hpp"
#include "wml_node.hpp"

namespace game_logic
{

class world;
		
class party
{
public:
	party(wml::const_node_ptr node, world& gameworld);
	virtual ~party() {}

	static party_ptr create_party(
					wml::const_node_ptr node,
					world& game_world);

	int id() const { return id_; }

	void draw();
	const hex::location& loc() const { return loc_; }
	void set_loc(const hex::location& loc) { loc_ = loc; }

	void new_world(world& w, const hex::location& loc);

	enum TURN_RESULT { TURN_COMPLETE, TURN_STILL_THINKING };
	TURN_RESULT play_turn();

	virtual bool is_human_controlled() const = 0;
	bool is_enemy(const party& p) const;

	game_time ready_to_move_at() const;

	void get_pos(GLfloat* pos) const;
	GLfloat get_rotation() const;

	const std::set<hex::location>& get_visible_locs() const;
	
	int vision() const;
	int track() const;

	int trackability() const;

	const std::vector<character_ptr>& members() { return members_; }
	bool is_destroyed() const;

	std::string status_text() const;
	std::string fatigue_status_text() const;

	const std::string& allegiance() const { return allegiance_; }

	virtual void friendly_encounter(party& p) {}

	const world& game_world() const { return *world_; }

	enum MOVEMENT_MODE { WALK = 1, RUN };
	void set_movement_mode(MOVEMENT_MODE mode) { move_mode_ = mode; }

	void full_heal();

	void join_party(character_ptr new_char);
	void merge_party(party& joining_party);

	void acquire_item(item_ptr i) {
		inventory_.push_back(i);
	}
	const std::vector<item_ptr>& inventory() const {
		return inventory_;
	}

	void assign_equipment(character_ptr c,
	                      int char_item, int party_item);

protected:
	void move(hex::DIRECTION dir);
	void pass(int minutes=1);
	int movement_cost(const hex::location& src,
	                  const hex::location& dst) const;
	const hex::gamemap& map() const;
	
private:
	int aggregate_stat_max(const std::string& stat) const;

	void apply_fatigue(const hex::location& src,
	                   const hex::location& dst);

	virtual TURN_RESULT do_turn() = 0;

	int id_;

	world* world_;
	hex::map_avatar_ptr avatar_;
	hex::location loc_, previous_loc_;
	hex::DIRECTION facing_, last_facing_;
	game_time departed_at_, arrive_at_;

	std::vector<character_ptr> members_;

	std::string allegiance_;

	mutable std::set<hex::location> visible_locs_;

	MOVEMENT_MODE move_mode_;

	std::vector<item_ptr> inventory_;
};

}

#endif