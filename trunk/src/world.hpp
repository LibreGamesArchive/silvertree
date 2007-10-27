
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef WORLD_HPP_INCLUDED
#define WORLD_HPP_INCLUDED

#include <functional>
#include <map>
#include <queue>
#include <string>

#include "camera.hpp"
#include "camera_controller.hpp"
#include "event_handler.hpp"
#include "formula.hpp"
#include "frame_rate_utils.hpp"
#include "gamemap.hpp"
#include "game_bar.hpp"
#include "game_time.hpp"
#include "grid_widget_fwd.hpp"
#include "particle_system.hpp"
#include "party.hpp"
#include "settlement_fwd.hpp"
#include "tracks.hpp"
#include "wml_node.hpp"
#include "world_fwd.hpp"

namespace game_logic
{

class world : public formula_callable
{
public:
	class new_game_exception {
	public:
		new_game_exception(const std::string& filename) : filename_(filename) {}
		const std::string& filename() { return filename_; }
	private:
		std::string filename_;
	};

	class quit_exception {
	};

	//function which will return the current stack of worlds the player is in.
	//The front of this vector will be the top-level world. The back of this vector will be
	//the current world.
	static const std::vector<const world*>& current_world_stack();
	explicit world(wml::const_node_ptr node);
	wml::node_ptr write() const;

	void play();

	void set_script(const std::string& script) { script_ = script; }

	const hex::gamemap& map() const { return map_; }
	const game_time& current_time() const { return time_; }
	void advance_time_until(const game_time& t) { time_ = t; }
	const GLfloat& subtime() const { return subtime_; }

	hex::camera& camera() { return camera_; }
	const hex::camera& camera() const { return camera_; }

	void get_parties_at(const hex::location& loc, std::vector<const_party_ptr>& chars) const;

	const_settlement_ptr settlement_at(const hex::location& loc) const;
	typedef std::multimap<hex::location,party_ptr> party_map;
	party_map::iterator add_party(party_ptr pty);
	void relocate_party(party_ptr party, const hex::location& loc);
	void get_matching_parties(const formula* filter,
	                          std::vector<party_ptr>& res);
	party_map& parties() { return parties_; }
	const party_map& parties() const { return parties_; }

	tracks& get_tracks() { return tracks_; }
	const tracks& get_tracks() const { return tracks_; }

	void fire_event(const std::string& name, const formula_callable& info);
	void add_event_handler(const std::string& event, const event_handler& handler);
	void draw() const;

	void quit() { done_ = true; quit_ = true; }

	void add_chat_label(gui::label_ptr label, const_character_ptr ch, int delay);
private:
	party_ptr get_pc_party() const;
	bool remove_party(party_ptr p);

	bool show_grid_;
	graphics::frame_rate_tracker fps_track_;
	graphics::texture compass_;

	gui::const_grid_ptr get_track_info() const;
	hex::gamemap map_;
	typedef std::pair<party_map::iterator,party_map::iterator>
	           party_map_range;
	typedef std::pair<party_map::const_iterator,
	                  party_map::const_iterator> const_party_map_range;
	party_map parties_;

	party_map::iterator find_party(const_party_ptr p);
	party_ptr get_party_ready_to_move();

	void set_lighting() const;

	struct party_orderer
	   : public std::binary_function<party_ptr,party_ptr,bool> {
		result_type operator()(first_argument_type p1,
		                       second_argument_type p2) const
		{
			return p1->ready_to_move_at() >
			       p2->ready_to_move_at();
		}
	};

	typedef std::priority_queue<party_ptr,std::vector<party_ptr>,
	                            party_orderer> party_queue;
	party_queue queue_;

	typedef std::map<hex::location,settlement_ptr> settlement_map;
	settlement_map settlements_;

	party_ptr focus_;

	mutable hex::camera camera_;
	mutable hex::camera_controller camera_controller_;

	mutable hex::location current_loc_;
	mutable std::vector<const hex::tile*> tiles_;
	mutable hex::tile::features_cache features_cache_;
	mutable gui::const_grid_ptr track_info_grid_;

	void rebuild_drawing_caches(const std::set<hex::location>& visible) const;

	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<formula_input>* inputs) const;

	game_time time_;
	GLfloat subtime_;
	tracks tracks_;

	std::map<hex::location,hex::location> exits_;

	const_formula_ptr sun_light_, ambient_light_, party_light_, party_light_power_;

	typedef std::multimap<std::string,event_handler> event_map;
	event_map handlers_;

	mutable graphics::particle_system particle_system_;
	game_dialogs::game_bar_ptr game_bar_;

	//if there is currently a set of scripted actions being run, the name
	//will be recorded here.
	std::string script_;

	std::string border_tile_;

	const hex::tile* get_tile_including_outside(const hex::location& loc) const;
	typedef std::map<hex::location,boost::shared_ptr<hex::tile> > outside_tiles_map;
	mutable outside_tiles_map outside_tiles_;
	bool done_;
	bool quit_;

	struct chat_label {
		gui::label_ptr label;
		int started_at;
		int y_loc;
		const_character_ptr character;
	};
	mutable std::vector<chat_label> chat_labels_;
};

}

#endif
