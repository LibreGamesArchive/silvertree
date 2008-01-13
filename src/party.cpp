
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
#include "formula_registry.hpp"
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

party::party(wml::const_node_ptr node, world& gameworld)
   : id_(wml::get_int(node,"unique_id",-1)),
     str_id_(wml::get_str(node,"id")),
     world_(&gameworld), loc_(wml::get_attr<int>(node,"x"),
     wml::get_attr<int>(node,"y")),
     facing_(hex::NORTH), last_facing_(hex::NORTH),
	 last_move_(hex::NULL_DIRECTION),
     arrive_at_(node),
	 allegiance_(wml::get_attr<std::string>(node,"allegiance")),
	 move_mode_(WALK), money_(wml::get_int(node,"money"))
{
	if(id_ == -1) {
		id_ = global_game_state::get().generate_new_party_id();
	}

	GLfloat pos[3];
	get_pos(pos);
	avatar_ = hex::map_avatar::create(node,pos);

	for(wml::node::const_child_range i =
	    node->get_child_range("character");
	    i.first != i.second; ++i.first) {
		const std::string& copies = i.first->second->attr("copies");
		int ncopies = 1;
		if(!copies.empty()) {
			try {
				ncopies = boost::lexical_cast<int>(copies);
			} catch(...) {
				std::cerr << "Could not convert '" << ncopies << "' to integer in specification of copies of a party\n";
			}
		}

		while(ncopies-- > 0) {
			members_.push_back(character::create(i.first->second));
		}

		if(!avatar_->valid()) {
			avatar_ = hex::map_avatar::create(members_.back()->write(),pos);
		}
	}

	const std::string items_csv = node->attr("items");
	if(!items_csv.empty()) {
		const std::vector<std::string> items = util::split(items_csv, ',');
		foreach(const std::string& id, items) {
			game_logic::const_item_ptr i = game_logic::item::get(id);
			if(!i) {
				std::cerr << "ERROR: unrecognized item: '" << id << "'\n";
				continue;
			}
			inventory_.push_back(game_logic::item_ptr(game_logic::item::get(id)->clone()));
		}
	}

	for(wml::node::const_child_range i =
	    node->get_child_range("item");
	    i.first != i.second; ++i.first) {
		inventory_.push_back(item::create_item(i.first->second));
	}

	//as a convenience users can place [encounter] tags within the [party]
	//and this will translate into a world [event] of type 'event' with a filter
	//for this party.
	for(wml::node::const_child_range i = node->get_child_range("encounter");
	    i.first != i.second; ++i.first) {
		wml::node_ptr event(new wml::node("event"));
		wml::copy_over(i.first->second, event);
		wml::node_ptr filter(new wml::node("filter"));
		filter->set_attr("filter", formatter() << "npc.unique_id = " << id_);
		event->add_child(filter);
		event_handler handler(event);
		gameworld.add_event_handler("encounter", handler);
	}
}

party::~party()
{
}

wml::node_ptr party::write() const
{
	wml::node_ptr res(new wml::node("party"));
	avatar_->write(res);
	res->set_attr("id", str_id_);
	res->set_attr("unique_id", formatter() << id_);
	res->set_attr("x", formatter() << loc_.x());
	res->set_attr("y", formatter() << loc_.y());
	WML_WRITE_ATTR(res, allegiance);
	WML_WRITE_ATTR(res, money);

	foreach(const_character_ptr c, members_) {
		res->add_child(c->write());
	}

	std::string items;
	foreach(const_item_ptr it, inventory_) {
		if(!items.empty()) {
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
	std::cerr << "entering world at " << loc.x() << "," << loc.y() << "\n";
	arrive_at_ = game_world().current_time();
	if(dir != hex::NULL_DIRECTION && movement_cost(loc_, hex::tile_in_direction(loc_, dir)) >= 0) {
		hex::location dst = hex::tile_in_direction(loc_, dir);
		move(dir);
	}

	enter_new_world(w);
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
	if(!scripted_moves_.empty()) {
		const hex::location& dst = scripted_moves_.front();
		if(dst == loc_) {
			scripted_moves_.pop_front();
			return play_turn();
		}

		hex::location adj[6];
		hex::get_adjacent_tiles(loc_, adj);
		int best = -1;
		for(int n = 0; n != 6; ++n) {
			if(movement_cost(loc_, adj[n]) < 0) {
				continue;
			}

			if(game_world().get_party_at(adj[n])) {
				continue;
			}

			if(best == -1 || hex::distance_between(adj[n], dst) < hex::distance_between(adj[best], dst)) {
				best = n;
			}
		}

		if(best == -1) {
			best = rand()%6;
			if(movement_cost(loc_, adj[best]) < 0) {
				pass();
				return TURN_COMPLETE;
			}
		}

		move(static_cast<hex::DIRECTION>(best));
		return TURN_COMPLETE;
	}

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

void party::acquire_item(item_ptr i)
{
	inventory_.push_back(i);
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
	arrive_at_ = game_world().current_time() + cost*game_world().scale();
	previous_loc_ = loc_;
	loc_ = dst;
	visible_locs_.clear();

	world_->fire_event("begin_move", *this);
}

void party::pass(int slices)
{
	last_facing_ = facing_;
	previous_loc_ = loc_;
	departed_at_ = game_world().current_time();
	arrive_at_ = game_world().current_time() + slices*game_world().scale();
	foreach(const character_ptr& c, members_) {
		c->use_stamina(-slices * 2);
	}

	const_formula_ptr heal_formula = formula_registry::get_stat_calculation("heal_amount");
	const int healing = heal();
	foreach(const character_ptr& c, members_) {
		if(heal_formula) {
			variant amount = heal_formula->execute(
							map_formula_callable_ptr(new map_formula_callable(c.get()))->add("heal",variant(healing)));
			c->heal(amount.as_int());
		}
	}
}

void party::finish_move()
{
	if(loc() != previous_loc()) {
		world_->fire_event("end_move", *this);
		previous_loc_ = loc_;
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

bool party::allowed_to_move(const hex::location& loc) const
{
	return map().is_loc_on_map(loc) &&
	       (get_visible_locs().count(loc) == 0 || !world_->get_party_at(loc));
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
		const int val = c->stat(stat);
		if(val > res) {
			res = val;
		}
	}

	return res;
}

int party::aggregate_stat_expert_and_assistant(const std::string& stat) const
{
	int scores[2] = {0, 0};
	foreach(const character_ptr& c, members_) {
		int val = c->stat(stat);
		if(val > scores[0]) {
			std::swap(scores[0], val);
		}

		if(val > scores[1]) {
			scores[1] = val;
		}
	}

	return scores[0] + scores[1]/2;
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
	return aggregate_stat_expert_and_assistant(Stat);
}

int party::track() const
{
	static const std::string Stat = "track";
	return aggregate_stat_expert_and_assistant(Stat);
}

int party::heal() const
{
	static const std::string Stat = "heal";
	return aggregate_stat_expert_and_assistant(Stat);
}

int party::haggle() const
{
	static const std::string Stat = "haggle";
	return aggregate_stat_expert_and_assistant(Stat);
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

void party::get_inputs(std::vector<formula_input>* inputs) const
{
	inputs->push_back(formula_input("allegiance", FORMULA_READ_WRITE));
	inputs->push_back(formula_input("party", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("var", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("is_npc", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("is_pc", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("world", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("id", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("unique_id", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("loc", FORMULA_READ_WRITE));
	inputs->push_back(formula_input("x", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("y", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("previous", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("members", FORMULA_READ_WRITE));
	inputs->push_back(formula_input("leader", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("money", FORMULA_READ_WRITE));
	inputs->push_back(formula_input("haggle", FORMULA_READ_ONLY));
	inputs->push_back(formula_input("var", FORMULA_READ_ONLY));
}

variant party::get_value(const std::string& key) const
{
	if(key == "allegiance") {
		return variant(allegiance());
	} else if(key == "party") {
		return variant(this);
	} else if(key == "var") {
		return variant(&global_game_state::get().get_variables());
	} else if(key == "is_npc") {
		return variant(is_human_controlled() ? 0 : 1);
	} else if(key == "is_pc") {
		return variant(is_human_controlled() ? 1 : 0);
	} else if(key == "world") {
		return variant(world_);
	} else if(key == "id") {
		return variant(str_id_);
	} else if(key == "unique_id") {
		return variant(id_);
	} else if(key == "loc") {
		return variant(new hex::location(loc_));
	} else if(key == "x") {
		return variant(loc_.x());
	} else if(key == "y") {
		return variant(loc_.y());
	} else if(key == "previous") {
		return variant(new hex::location(previous_loc_));
	} else if(key == "members") {
		std::vector<variant> members;
		foreach(const character_ptr& c, members_) {
			members.push_back(variant(c.get()));
		}
		return variant(&members);
	} else if(key == "leader") {
		assert(!members_.empty());
		return variant(members_.front().get());
	} else if(key == "money") {
		return variant(money());
	} else if(key == "haggle") {
		return variant(haggle());
	} else if(key == str_id_) {
		return variant(id_);
	}

	return variant();
}

void party::set_value(const std::string& key, const variant& value)
{
	if(key == "allegiance") {
		allegiance_ = value.as_string();
	} else if(key == "members") {
		members_.clear();
		for(int n = 0; n != value.num_elements(); ++n) {
			variant val = value[n];
			if(val.is_callable()) {
				character* c = dynamic_cast<character*>(val.mutable_callable());
				if(c) {
					members_.push_back(character_ptr(c));
				}
			}
		}
	} else if(key == "money") {
		money_ = value.as_int();
	} else if(key == "loc") {
		game_world().relocate_party(this, *value.convert_to<hex::location>());
		previous_loc_ = loc_;
	}
}

}
