
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BATTLE_CHARACTER_HPP_INCLUDED
#define BATTLE_CHARACTER_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <gl.h>

#include <set>
#include <vector>

#include "battle_character_fwd.hpp"
#include "battle_move_fwd.hpp"
#include "character_fwd.hpp"
#include "gamemap.hpp"
#include "map_avatar.hpp"
#include "model.hpp"
#include "party.hpp"
#include "tile_logic.hpp"
#include "location_tracker.hpp"

namespace game_logic
{

class battle;
class game_time;

class battle_character
{
public:
	battle_character(character_ptr ch, const party& p,
	                 const hex::location& loc, hex::DIRECTION facing,
					 const hex::gamemap& map, const game_time& time);
	virtual ~battle_character() {}

	static battle_character_ptr
	make_battle_character(character_ptr ch, const party& p,
	                 const hex::location& loc, hex::DIRECTION facing,
					 const hex::gamemap& map, const game_time& time);

	character& get_character() const { return *char_; }
	const party& get_party() const { return party_; }
	const hex::location& loc() const { return loc_; }
	hex::DIRECTION facing() const { return facing_; }

	void draw() const;

	void play_turn(battle& b) { do_turn(b); }
	bool is_human_controlled() const { return is_human(); }

	typedef std::vector<hex::location> route;
	typedef std::map<hex::location,route> move_map;
	void get_possible_moves(
	        move_map& locs,
	        const battle_move& move,
	        const std::vector<battle_character_ptr>& chars) const;
	void reset_movement_plan();
	const route& movement_plan() const { return move_; }
	bool move(hex::DIRECTION dir);

	void get_pos(GLfloat* pos, GLfloat* rotate) const;
	void get_pos_during_move(GLfloat* pos, GLfloat* rotate, GLfloat time) const;

	hex::location get_loc_during_move(int time) const;

	void set_time_during_move(GLfloat time_in_move) {
		time_in_move_ = time_in_move;
	}

	void stop_move_at(int time);
	int arrival_time(const hex::location& loc) const;

	void commit_move();

	bool is_enemy(const battle_character& c) const;
	bool can_attack(const battle_character& c,
			const std::vector<battle_character_ptr>& chars,
	                hex::location loc=hex::location(),
					bool draw=false) const;

	int ready_to_move_at() const { return move_at_; }

	void begin_facing_change(hex::DIRECTION facing) {
		facing_ = facing;
	}
	void end_facing_change() {
		old_facing_ = facing_;
		assert(old_facing_ >= hex::NORTH && old_facing_ <= hex::NULL_DIRECTION);
	}

	GLfloat begin_move(const route& move) { return route_cost(move_ = move); }
	void set_movement_time(GLfloat time) { time_in_move_ = time; }
	GLfloat get_movement_time() { return time_in_move_; }
	void end_move();

	void begin_attack(const battle_character& enemy);
	bool set_attack_time(GLfloat time) { return time > 3.0 && time < 4.0; }
	void end_attack();

	void set_highlight(const GLfloat* highlight) { highlight_ = highlight; }
	virtual bool is_human() const = 0;

	int attack() const;
	int defense() const;
	int defense(const std::string& damage_type) const;
	int stat(const std::string& s) const;
	int mod_stat(const std::string& s) const;
	std::string status_text() const;

	void set_time_until_next_move(int amount) { move_at_ += amount; }

	void update_time(int cur_time);
	void add_modification(const std::string& stat, int expire, int mod);
	const graphics::location_tracker& loc_tracker() { return loc_tracker_; }
	int route_cost(const route& r) const;
private:
	void get_possible_moves_internal(move_map& locs, const std::vector<battle_character_ptr>& chars, route& r, int max_distance) const;

	virtual void do_turn(battle& b) = 0;
	
	character_ptr const char_;
	const party& party_;
	hex::location loc_;
	hex::DIRECTION facing_;
	hex::DIRECTION old_facing_;
	int move_at_;

	int planned_move_cost() const;
	int move_cost(const hex::location& a, const hex::location& b) const;
	int adjust_damage(int damage) const;
	route move_;
	GLfloat time_in_move_;
	
	const hex::gamemap& map_;

	const GLfloat* highlight_;

	int time_of_day_adjustment_;

	mutable graphics::location_tracker loc_tracker_;

	struct stat_mod {
		int expire;
		int mod;
	};

	std::multimap<std::string,stat_mod> mods_;
};

}

#endif
