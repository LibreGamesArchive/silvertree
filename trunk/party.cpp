
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "character.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "global_game_state.hpp"
#include "graphics_logic.hpp"
#include "item.hpp"
#include "npc_party.hpp"
#include "party.hpp"
#include "pc_party.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"
#include "tile.hpp"
#include "tile_logic.hpp"
#include "world.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

#include <cmath>

#include <iostream>

namespace game_logic
{

namespace {
int current_id = 1;
}

party::party(wml::const_node_ptr node, world& gameworld)
   : id_(current_id++), str_id_(wml::get_str(node,"id")),
     world_(&gameworld), loc_(wml::get_attr<int>(node,"x"),
     wml::get_attr<int>(node,"y")),
     facing_(hex::NORTH), last_facing_(hex::NORTH),
	 last_move_(hex::NULL_DIRECTION),
     arrive_at_(node),
	 allegiance_(wml::get_attr<std::string>(node,"allegiance")),
	 move_mode_(WALK), money_(wml::get_int(node,"money"))
{
	GLfloat pos[3];
	get_pos(pos);
	avatar_ = hex::map_avatar::create(node,pos);

	for(wml::node::const_child_range i =
	    node->get_child_range("character");
	    i.first != i.second; ++i.first) {
		members_.push_back(character::create(i.first->second));
	}

	const std::string items_csv = node->attr("items");
	if(!items_csv.empty()) {
		const std::vector<std::string> items = util::split(items_csv, ',');
		foreach(const std::string& id, items) {
			inventory_.push_back(game_logic::item_ptr(game_logic::item::get(id)->clone()));
		}
	}

	for(wml::node::const_child_range i =
	    node->get_child_range("item");
	    i.first != i.second; ++i.first) {
		inventory_.push_back(item::create_item(i.first->second));
	}
}

wml::node_ptr party::write() const
{
	wml::node_ptr res(new wml::node("party"));
	avatar_->write(res);
	res->set_attr("id", str_id_);
	res->set_attr("x", boost::lexical_cast<std::string>(loc_.x()));
	res->set_attr("y", boost::lexical_cast<std::string>(loc_.y()));
	WML_WRITE_ATTR(res, allegiance);
	WML_WRITE_ATTR(res, money);

	foreach(const_character_ptr c, members_) {
		res->add_child(c->write());
	}

	std::string items;
	foreach(const_item_ptr it, inventory_) {
		if(items.empty()) {
			items += ",";
		}

		items += it->id();
	}

	res->set_attr("items", items);
	
	return res;
}

void party::new_world(world& w, const hex::location& loc, hex::DIRECTION dir)
{
	world_ = &w;
	loc_ = loc;
	arrive_at_ = game_world().current_time();
	if(dir != hex::NULL_DIRECTION) {
		move(dir);
	}
}

party_ptr party::create_party(wml::const_node_ptr node,
                              world& game_world)
{
	if((*node)["controller"] == "human") {
		return party_ptr(new pc_party(node,game_world));
	} else {
		return party_ptr(new npc_party(node,game_world));
	}
}

void party::draw()
{
	avatar_->set_rotation(get_rotation());
	get_pos(avatar_->position_buffer());
	avatar_->draw();
}

party::TURN_RESULT party::play_turn()
{
	return do_turn();
}

bool party::is_enemy(const party& p) const
{
	return allegiance() != p.allegiance() && !preference_nocombat();
}

game_time party::ready_to_move_at() const
{
	return arrive_at_;
}

void party::full_heal()
{
	foreach(character_ptr& c, members_) {
		c->full_heal();
	}
}

void party::join_party(character_ptr new_char)
{
	members_.push_back(new_char);
}

void party::merge_party(party& joining_party)
{
	foreach(character_ptr c, joining_party.members_) {
		join_party(c);
	}

	joining_party.members_.clear();
}

void party::move(hex::DIRECTION dir)
{
	last_move_ = dir;
	world_->get_tracks().add_tracks(loc(), *this, world_->current_time(), dir);
	last_facing_ = facing_;
	facing_ = dir;
	const hex::location dst = hex::tile_in_direction(loc(),dir);
	const int cost = movement_cost(loc(),dst)/move_mode_;
	apply_fatigue(loc(),dst);
	departed_at_ = game_world().current_time();
	arrive_at_ = game_world().current_time() + cost;
	previous_loc_ = loc_;
	loc_ = dst;
	visible_locs_.clear();

	world_->fire_event("begin_move", *this);
}

void party::pass(int minutes)
{
	last_facing_ = facing_;
	previous_loc_ = loc_;
	departed_at_ = game_world().current_time();
	arrive_at_ = game_world().current_time() + minutes;
	foreach(const character_ptr& c, members_) {
		c->use_stamina(-minutes * 2);
	}
}

int party::movement_cost(const hex::location& src,
                         const hex::location& dst) const
{
	if(map().is_loc_on_map(src) == false ||
	   map().is_loc_on_map(dst) == false) {
		return -1;
	}

	const hex::tile& t1 = map().get_tile(src);
	const hex::tile& t2 = map().get_tile(dst);

	if(!t1.is_passable(hex::get_adjacent_direction(src, dst))) {
		return -1;
	}

	const int gradient = t2.height() - t1.height();
	const hex::const_base_terrain_ptr terrain = t2.terrain();
	const hex::const_terrain_feature_ptr feature = t2.feature();

	int res = -1;
	for(std::vector<character_ptr>::const_iterator i = members_.begin();
	    i != members_.end(); ++i) {
		const int cost = (*i)->move_cost(terrain,feature,gradient);
		if(cost < 0) {
			return -1;
		}

		if(res == -1 || cost > res) {
			res = cost;
		}
	}
	
	return res;
}

void party::apply_fatigue(const hex::location& src,
                          const hex::location& dst)
{
	assert(map().is_loc_on_map(src));
	assert(map().is_loc_on_map(dst));
	const hex::tile& t1 = map().get_tile(src);
	const hex::tile& t2 = map().get_tile(dst);
	const int gradient = t2.height() - t1.height();
	const hex::const_base_terrain_ptr terrain = t2.terrain();
	const hex::const_terrain_feature_ptr feature = t2.feature();

	foreach(const character_ptr& c, members_) {
		const int cost = c->move_cost(terrain,feature,gradient)*move_mode_*move_mode_;
		assert(cost != -1);
		c->use_stamina(cost);
	}
}

const hex::gamemap& party::map() const
{
	return world_->map();
}

int party::aggregate_stat_max(const std::string& stat) const
{
	int res = 0;
	foreach(const character_ptr& c, members_) {
		res += c->stat(stat);
	}

	return res;
}

void party::get_pos(GLfloat* pos) const
{
	using hex::tile;

	if(arrive_at_ > game_world().current_time() &&
	   map().is_loc_on_map(previous_loc_)) {
		const GLfloat depart =
		       static_cast<GLfloat>(departed_at_.since_epoch());
		const GLfloat arrive =
		       static_cast<GLfloat>(arrive_at_.since_epoch());
		const GLfloat current = static_cast<GLfloat>(
		        game_world().current_time().since_epoch()) +
		        game_world().subtime();

		const GLfloat total = arrive - depart;
		const GLfloat new_factor = (current - depart)/total;
		const GLfloat old_factor = (arrive - current)/total;

		pos[0] = tile::translate_x(loc_)*new_factor +
		         tile::translate_x(previous_loc_)*old_factor;
		pos[1] = tile::translate_y(loc_)*new_factor +
		         tile::translate_y(previous_loc_)*old_factor;

		if(map().is_loc_on_map(loc_) &&
		   map().is_loc_on_map(previous_loc_)) {
			pos[2] = hex::tile::translate_height(
			      map().get_tile(loc_).height())*new_factor +
			         hex::tile::translate_height(
			      map().get_tile(previous_loc_).height())*old_factor;
		} else {
			pos[2] = 0.0;
		}
	} else {
	
		pos[0] = tile::translate_x(loc_);
		pos[1] = tile::translate_y(loc_);

		if(map().is_loc_on_map(loc_)) {
			pos[2] = hex::tile::translate_height(
			                       map().get_tile(loc_).height());
		} else {
			pos[2] = 0.0;
		}
	}
}

GLfloat party::get_rotation() const
{
	const int dir = static_cast<int>(facing_);
	GLfloat res = dir*60.0;
	if(last_facing_ != facing_ &&
	   game_world().current_time() == departed_at_) {
		const int dir = static_cast<int>(last_facing_);
		return graphics::calculate_rotation(dir*60.0,res,game_world().subtime());
	}

	return res;
}

const std::set<hex::location>& party::get_visible_locs() const
{
	if(visible_locs_.empty() == false) {
		return visible_locs_;
	}

	const int start_ticks = SDL_GetTicks();

	const int range = vision();

	visible_locs_.insert(loc_);
	bool found = true;
	for(int radius = 1; found && radius < range; ++radius) {
		found = false;
		std::vector<hex::location> locs;
		hex::get_tile_ring(loc_,radius,locs);
		for(std::vector<hex::location>::const_iterator i = locs.begin();
		    i != locs.end(); ++i) {
			if(line_of_sight(world_->map(),loc_,*i)) {
				found = true;
				visible_locs_.insert(*i);
			}
		}
	}

	const int taken = SDL_GetTicks() - start_ticks;
	std::cerr << "get_visible_locs(): " << taken << "\n";

	return visible_locs_;
}

void party::get_visible_parties(std::vector<const_party_ptr>& parties) const
{
	const int range = vision();
	world::party_map& all = world_->parties();
	for(world::party_map::iterator i = all.begin(); i != all.end(); ++i) {
		if(range >= hex::distance_between(loc_,i->first)) {
			if(hex::line_of_sight(world_->map(),loc_,i->first)) {
				parties.push_back(i->second);
			}
		}
	}
}

int party::vision() const
{
	static const std::string Stat = "vision";
	return aggregate_stat_max(Stat);
}

int party::track() const
{
	static const std::string Stat = "track";
	return aggregate_stat_max(Stat);
}

int party::trackability() const
{
	return members_.size()*100;
}

void party::destroy()
{
	members_.clear();
}

bool party::is_destroyed() const
{
	foreach(const character_ptr& c, members_) {
		if(c->dead() == false) {
			return false;
		}
	}

	return true;
}

std::string party::status_text() const
{
	std::ostringstream s;
	foreach(const character_ptr& c, members_) {
		s << c->description() << " level " << c->level()
		  << "\n HP: " << c->hitpoints() << "/" << c->max_hitpoints()
		  << "\n XP: " << c->experience() << "/"
		  << c->experience_required()
		  << "\n Fatigue: " << (c->fatigue()/10)
		  << "/" << (c->stamina()/10) << "\n";
	}

	return s.str();
}

std::string party::fatigue_status_text() const
{
	std::ostringstream s;
	foreach(const character_ptr& c, members_) {
		s << c->description() << ": " << (c->fatigue()/10) << "/"
		  << (c->stamina()/10) << "\n";
	}

	return s.str();
}

void party::assign_equipment(character_ptr c,
                             int char_item, int party_item)
{
	assert(party_item >= 0 && party_item < inventory_.size());
	c->swap_equipment(char_item, inventory_[party_item]);
	if(inventory_[party_item]->is_null()) {
		inventory_.erase(inventory_.begin()+party_item);
	}
}

variant party::get_value(const std::string& key) const
{
	if(key == "var") {
		return variant(&global_game_state::get().get_variables());
	} else if(key == "is_npc") {
		return variant(is_human_controlled() ? 0 : 1);
	} else if(key == "is_pc") {
		return variant(is_human_controlled() ? 1 : 0);
	} else if(key == "world") {
		return variant(world_);
	} else if(key == "id") {
		return variant(id_);
	} else if(key == "loc") {
		return variant(&loc_);
	} else if(key == "previous") {
		return variant(&previous_loc_);
	} else if(key == str_id_) {
		return variant(id_);
	} else {
		return variant();
	}
}

}
