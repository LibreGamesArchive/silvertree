
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "battle.hpp"
#include "battle_character.hpp"
#include "battle_character_npc.hpp"
#include "battle_character_pc.hpp"
#include "battle_move.hpp"
#include "character.hpp"
#include "foreach.hpp"
#include "graphics_logic.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "tile.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

namespace game_logic
{

battle_character::battle_character(
          character_ptr ch, const party& p,
          const hex::location& loc, hex::DIRECTION facing,
		  const hex::gamemap& map, const game_time& time)
 : char_(ch), party_(p), loc_(loc),
   facing_(facing), old_facing_(facing),
   move_at_(ch->initiative()), time_in_move_(-1.0), map_(map),
   highlight_(NULL),
   time_of_day_adjustment_(ch->alignment()*time.alignment_adjustment())
{
	assert(old_facing_ >= hex::NORTH && old_facing_ <= hex::NULL_DIRECTION);
}

battle_character_ptr battle_character::make_battle_character(
          character_ptr ch, const party& p,
          const hex::location& loc, hex::DIRECTION facing,
		  const hex::gamemap& map, const game_time& time)
{
	if(p.is_human_controlled()) {
		return battle_character_ptr(
		         new battle_character_pc(ch,p,loc,facing,map,time));
	} else {
		return battle_character_ptr(
		         new battle_character_npc(ch,p,loc,facing,map,time));
	}
}

void battle_character::draw() const
{
	GLfloat pos[3];
	GLfloat rotate;
	get_pos(pos, &rotate);

	graphics::surface surf = graphics::surface_cache::get(
	                                            char_->image());
	if(surf.get() == NULL) {
		return;
	}

	glPushMatrix();
	glTranslatef(pos[0],pos[1],pos[2]);
	glRotatef(rotate,0.0,0.0,1.0);

	if(highlight_) {
		glDisable(GL_LIGHT0);
		glEnable(GL_LIGHT3);
		glLightfv(GL_LIGHT3,GL_AMBIENT,highlight_);
		glLightfv(GL_LIGHT3,GL_DIFFUSE,highlight_);
	}

	std::vector<graphics::surface> surfs;
	surfs.push_back(surf);

	graphics::texture::set_current_texture(surfs);

	glBegin(GL_QUADS);

	graphics::texture::set_coord(0.0,0.0);
	glVertex3f(-0.5,0.0,1.0);

	graphics::texture::set_coord(1.0,0.0);
	glVertex3f(0.5,0.0,1.0);

	graphics::texture::set_coord(1.0,1.0);
	glVertex3f(0.5,0.0,0.0);

	graphics::texture::set_coord(0.0,1.0);
	glVertex3f(-0.5,0.0,0.0);

	glEnd();

	if(highlight_) {
		glEnable(GL_LIGHT0);
		glDisable(GL_LIGHT3);
	}

	glPopMatrix();
}

void battle_character::get_pos(GLfloat* pos, GLfloat* rotate) const
{
	if(time_in_move_ >= 0.0 && !move_.empty()) {
		get_pos_during_move(pos,rotate,time_in_move_);
		return;
	}

	using hex::tile;
	pos[0] = tile::translate_x(loc_);
	pos[1] = tile::translate_y(loc_);
	if(map_.is_loc_on_map(loc_)) {
		pos[2] = tile::translate_height(
		                  map_.get_tile(loc_).height());
	}

	*rotate = facing_*60.0;

	//if the unit is rotating
	if(time_in_move_ >= 0.0) {
		const GLfloat rotate_time = 1.0;
		if(time_in_move_ < rotate_time) {
			*rotate = graphics::calculate_rotation(
			  old_facing_*60.0,facing_*60.0,time_in_move_/rotate_time);
		}
	}
}

void battle_character::get_pos_during_move(GLfloat* pos,
                                           GLfloat* rotate,
                                           GLfloat time) const
{
	assert(move_.empty() == false);

	unsigned int index = 0;
	int cost = 0;
	while(index < move_.size()-1) {
		cost = move_cost(move_[index],move_[index+1]);
		if(cost > time) {
			break;
		} else {
			time -= cost;
		}

		++index;
	}

	const GLfloat ratio2 = (cost <= 0) ? 0 : time/cost;
	const GLfloat ratio1 = 1.0 - ratio2;
	const hex::location loc0 = index == 0 ? move_[index] : move_[index-1];
	const hex::location loc1 = move_[index];
	const hex::location loc2 = index+1 < move_.size() ?
	                           move_[index+1] : move_[index];
	assert(map_.is_loc_on_map(loc1) &&
	       map_.is_loc_on_map(loc2));

	using hex::tile;
	const tile& t1 = map_.get_tile(loc1);
	const tile& t2 = map_.get_tile(loc2);
	pos[0] = ratio1*tile::translate_x(loc1) +
	         ratio2*tile::translate_x(loc2);
	pos[1] = ratio1*tile::translate_y(loc1) +
	         ratio2*tile::translate_y(loc2);
	pos[2] = ratio1*tile::translate_height(t1.height()) +
	         ratio2*tile::translate_height(t2.height());

	const GLfloat rotate1 = get_adjacent_direction(loc0,loc1)*60.0;
	const GLfloat rotate2 = get_adjacent_direction(loc1,loc2)*60.0;
	*rotate = graphics::calculate_rotation(rotate1,rotate2,ratio1);
}

hex::location battle_character::get_loc_during_move(int time) const
{
	assert(move_.empty() == false);

	unsigned int index = 0;
	int cost = 0;
	while(index < move_.size()-1) {
		cost = move_cost(move_[index],move_[index+1]);
		if(cost > time) {
			break;
		} else {
			time -= cost;
		}

		++index;
	}

	return move_[index];
}

void battle_character::get_possible_moves(
    battle_character::move_map& moves,
	const battle_move& move,
	const std::vector<battle_character_ptr>& chars) const
{
	moves.clear();

	route r;
	r.push_back(loc_);
	get_possible_moves_internal(moves, chars, r, move.max_moves());

	battle_character::move_map::iterator i = moves.begin();
	while(i != moves.end()) {
		if(move.must_attack()) {
			bool found = false;
			foreach(const battle_character_ptr& c, chars) {
				if(is_enemy(*c) && can_attack(*c, i->first)) {
					found = true;
					break;
				}
			}

			if(!found) {
				 moves.erase(i++);
				continue;
			}
		}

		if(hex::distance_between(loc_,i->first) < move.min_moves()) {
			moves.erase(i++);
		} else {
			++i;
		}
	}
}

void battle_character::get_possible_moves_internal(battle_character::move_map& locs, const std::vector<battle_character_ptr>& chars, battle_character::route& r, int max_distance) const
{
	const int cost = route_cost(r);
	if(cost > battle::movement_duration()) {
		return;
	}

	move_map::iterator cur = locs.find(r.back());
	if(cur != locs.end() && route_cost(cur->second) <= cost) {
		return;
	}

	locs[r.back()] = r;

	if(r.size() == max_distance+1) {
		return;
	}

	hex::location adj[6];
	get_adjacent_tiles(r.back(), adj);
	for(int n = 0; n != 6; ++n) {
		bool occupied = false;
		foreach(const battle_character_ptr& c, chars) {
			if(c->loc() == adj[n]) {
				occupied = true;
				break;
			}
		}
		if(map_.is_loc_on_map(adj[n]) == false || occupied || distance_between(adj[n],r.front()) > max_distance) {
			continue;
		}
		r.push_back(adj[n]);
		get_possible_moves_internal(locs, chars, r, max_distance);
		r.pop_back();
	}
}

int battle_character::route_cost(const route& r) const
{
	int res = 0;
	for(unsigned int i = 0; i < r.size()-1; ++i) {
		const int cost = move_cost(r[i],r[i+1]);
		if(cost < 0) {
			return 100000;
		}
		res += cost;
	}

	return res;
}

void battle_character::stop_move_at(int time)
{
	const hex::location loc = get_loc_during_move(time);
	route::iterator i = std::find(move_.begin(),move_.end(),loc);
	if(i == move_.end()) {
		return;
	}
	move_.erase(i+1,move_.end());
}

int battle_character::arrival_time(const hex::location& loc) const
{
	int res = 0;
	for(unsigned int n = 0; n != move_.size(); ++n) {
		if(n > 0) {
			res += move_cost(move_[n-1],move_[n]);
		}

		if(move_[n] == loc) {
			return res;
		}
	}

	return -1;
}

void battle_character::commit_move()
{
	assert(move_.empty() == false);
	loc_ = move_.back();
	reset_movement_plan();
}

bool battle_character::is_enemy(const battle_character& c) const
{
	return is_human_controlled() != c.is_human_controlled();
}

bool battle_character::can_attack(const battle_character& c,
                                  hex::location loc, bool draw) const
{
	if(!loc.valid()) {
		loc = loc_;
	}

	const int range = char_->attack_range();
	if(range > 1) {
		std::vector<const hex::tile*> line;
		if(hex::line_of_sight(map_,loc,c.loc(),&line,range,draw)) {
			return true;
		} else {
			return false;
		}
	} else {
		return tiles_adjacent(loc,c.loc());
	}
}

void battle_character::end_move()
{
	set_time_until_next_move(route_cost(move_));
	time_in_move_ = -1.0;
	loc_ = move_.back();
	if(move_.size() > 1) {
		old_facing_ = facing_ = get_adjacent_direction(move_[move_.size()-2],move_.back());
		assert(old_facing_ >= hex::NORTH && old_facing_ <= hex::NULL_DIRECTION);
	}
	move_.clear();
}

void battle_character::begin_attack(const battle_character& enemy)
{
	old_facing_ = facing_ = get_adjacent_direction(loc_, enemy.loc());
		assert(old_facing_ >= hex::NORTH && old_facing_ <= hex::NULL_DIRECTION);
}

void battle_character::end_attack()
{
}

int battle_character::attack() const
{
	return get_character().attack() + mod_stat("attack");
}

int battle_character::defense() const
{
	return get_character().defense() + mod_stat("defense");
}

int battle_character::defense(const std::string& damage_type) const
{
	return get_character().defense(damage_type) + mod_stat("defense");
}

int battle_character::stat(const std::string& s) const
{
	return get_character().stat(s) + mod_stat(s);
}

int battle_character::mod_stat(const std::string& s) const
{
	int res = 0;
	typedef std::multimap<std::string,stat_mod>::const_iterator itor;
	std::pair<itor,itor> range(mods_.equal_range(s));
	while(range.first != range.second) {
		res += range.first->second.mod;
		++range.first;
	}

	return res;
}

std::string battle_character::status_text() const
{
	std::ostringstream s;
	s << char_->description() << "\n"
	  << "HP: " << char_->hitpoints() << "/"
	  << char_->max_hitpoints() << "\n"
	  << "Attack: " << attack() << "\n"
	  << "Defense: " << defense() << "\n"
	  << "Damage: " << adjust_damage(char_->damage()) << "\n"
	  << char_->alignment_description() << "("
	  << (time_of_day_adjustment_ > 0 ? "+" : "")
	  << time_of_day_adjustment_ << "%)";
	return s.str();
}

void battle_character::reset_movement_plan()
{
	move_.clear();
	move_.push_back(loc_);
	time_in_move_ = -1.0;
}

bool battle_character::move(hex::DIRECTION dir)
{
	assert(!move_.empty());

	const hex::location loc = hex::tile_in_direction(move_.back(),dir);
	const route::iterator i = std::find(move_.begin(),move_.end(),loc);
	if(i != move_.end()) {
		move_.erase(i+1,move_.end());
		return true;
	}
	
	const int cost = move_cost(move_.back(),loc);
	if(cost == -1 ||
	   planned_move_cost() + cost > battle::movement_duration()) {
		return false;
	}

	move_.push_back(loc);
	return true;
}

int battle_character::planned_move_cost() const
{
	int res = 0;
	for(size_t n = 0; n < move_.size()-1; ++n) {
		res += move_cost(move_[n],move_[n+1]);
	}

	return res;
}

int battle_character::move_cost(const hex::location& a,
                                const hex::location& b) const
{
	if(map_.is_loc_on_map(a) == false ||
	   map_.is_loc_on_map(b) == false) {
		return -1;
	}

	const hex::tile& t1 = map_.get_tile(a);
	const hex::tile& t2 = map_.get_tile(b);

	const int height_diff = t2.height() - t1.height();

	return char_->move_cost(t2.terrain(),t2.feature(),height_diff);
}

int battle_character::adjust_damage(int damage) const
{
	const int adjust = (time_of_day_adjustment_ < 0 ? -1 : 1) *
	                   ((abs(time_of_day_adjustment_)*damage)/100);
	return damage + adjust;
}

void battle_character::update_time(int time)
{
	std::multimap<std::string,stat_mod>::iterator i = mods_.begin();
	while(i != mods_.end()) {
		if(time >= i->second.expire) {
			mods_.erase(i++);
		} else {
			++i;
		}
	}
}

void battle_character::add_modification(const std::string& stat,
                                        int expire, int mod)
{
	std::multimap<std::string,stat_mod>::iterator i = mods_.insert(std::pair<std::string,stat_mod>(stat,stat_mod()));
	i->second.expire = expire;
	i->second.mod = mod;
}

}
