
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include "encounter.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "frustum.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "party_status_dialog.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "settlement.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "world.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

namespace game_logic
{

namespace {
std::string get_map_data(wml::const_node_ptr node)
{
	const std::string& data = (*node)["map_data"];
	if(!data.empty()) {
		return data;
	}

	return sys::read_file((*node)["map"]);
}

std::vector<const world*> world_stack;
struct world_context {
	explicit world_context(const world* w) {
		world_stack.push_back(w);
	}

	~world_context() {
		assert(!world_stack.empty());
		world_stack.pop_back();
	}
};

}

const std::vector<const world*>& world::current_world_stack()
{
	return world_stack;
}

world::world(wml::const_node_ptr node)
	: compass_(graphics::texture::get(graphics::surface_cache::get("compass-rose.png"))),
	  map_(get_map_data(node)), camera_(map_),
	  camera_controller_(camera_),
	  time_(node), subtime_(0.0), tracks_(map_)
{
	show_grid_ = false;

	const std::string& sun_light = wml::get_str(node, "sun_light");
	if(!sun_light.empty()) {
		sun_light_.reset(new formula(sun_light));
	}

	const std::string& ambient_light = wml::get_str(node, "ambient_light");
	if(!ambient_light.empty()) {
		ambient_light_.reset(new formula(ambient_light));
	}

	wml::node::const_child_iterator p1 = node->begin_child("party");
	const wml::node::const_child_iterator p2 = node->end_child("party");
	for(; p1 != p2; ++p1) {
		party_ptr new_party(party::create_party(p1->second,*this));
		if(new_party) {
			add_party(new_party);
		}
	}

	wml::node::const_child_iterator s1 = node->begin_child("settlement");
	const wml::node::const_child_iterator s2 = node->end_child("settlement");
	for(; s1 != s2; ++s1) {
		wml::node_ptr node = wml::deep_copy(s1->second);
		const std::string& file = node->attr("file");
		if(!file.empty()) {
			wml::merge_over(wml::parse_wml(sys::read_file(file)), node);
		}
		settlement_ptr s(new settlement(node,map_));
		std::vector<hex::location> locs;
		s->entry_points(locs);
		foreach(const hex::location& loc, locs) {
			settlements_[loc] = s;
		}
	}

	wml::node::const_child_iterator e1 = node->begin_child("exit");
	const wml::node::const_child_iterator e2 = node->end_child("exit");
	for(; e1 != e2; ++e1) {
		hex::location loc1(wml::get_attr<int>(e1->second,"x"),
		                   wml::get_attr<int>(e1->second,"y"));
		hex::location loc2(wml::get_attr<int>(e1->second,"xdst",-1),
		                   wml::get_attr<int>(e1->second,"ydst",-1));
		exits_[loc1] = loc2;
	}

	const std::vector<wml::const_node_ptr> portals = wml::child_nodes(node, "portal");
	foreach(const wml::const_node_ptr& portal, portals) {
		hex::location loc1(wml::get_attr<int>(portal,"xdst"),
		                   wml::get_attr<int>(portal,"ydst"));
		hex::location loc2(wml::get_attr<int>(portal,"xsrc"),
		                   wml::get_attr<int>(portal,"ysrc"));
		exits_[loc1] = loc2;
	}
}

wml::node_ptr world::write() const
{
	wml::node_ptr res(new wml::node("scenario"));
	res->set_attr("time", boost::lexical_cast<std::string>(time_.since_epoch()));
	res->set_attr("map_data", map_.write());
	if(sun_light_) {
		res->set_attr("sun_light", sun_light_->str());
	}

	if(ambient_light_) {
		res->set_attr("ambient_light", ambient_light_->str());
	}

	for(party_map::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
		res->add_child(i->second->write());
	}

	for(std::map<hex::location,hex::location>::const_iterator i = exits_.begin(); i != exits_.end(); ++i) {
		wml::node_ptr portal(new wml::node("portal"));
		portal->set_attr("xdst", boost::lexical_cast<std::string>(i->first.x()));
		portal->set_attr("ydst", boost::lexical_cast<std::string>(i->first.y()));
		portal->set_attr("xsrc", boost::lexical_cast<std::string>(i->second.x()));
		portal->set_attr("ysrc", boost::lexical_cast<std::string>(i->second.y()));
		res->add_child(portal);
	}

	for(settlement_map::const_iterator i = settlements_.begin(); i != settlements_.end(); ++i) {
		res->add_child(i->second->write());
	}

	return res;
}

void world::get_parties_at(const hex::location& loc, std::vector<const_party_ptr>& chars) const
{
	const_party_map_range range = parties_.equal_range(loc);
	while(range.first != range.second) {
		chars.push_back(range.first->second);
		++range.first;
	}
}

const_settlement_ptr world::settlement_at(const hex::location& loc) const
{
	const settlement_map::const_iterator i = settlements_.find(loc);
	if(i != settlements_.end()) {
		return i->second;
	} else {
		return const_settlement_ptr();
	}
}

world::party_map::iterator world::add_party(party_ptr new_party)
{
	party_map::iterator res = parties_.insert(std::pair<hex::location,party_ptr>(
	                 new_party->loc(),new_party));
	queue_.push(new_party);

	std::cerr << "added party at " << new_party->loc().x() << "," << new_party->loc().y() << "\n";
	if(new_party->is_human_controlled()) {
		std::cerr << "is human\n";
		focus_ = new_party;
		game_bar_.reset(new game_dialogs::game_bar(0, graphics::screen_height()-128, 
							   graphics::screen_width(), 128, this, new_party));
		game_bar_->set_frame(gui::frame_manager::make_frame(game_bar_, "game-bar-frame"));
	}

	return res;
}

void world::get_matching_parties(const formula_ptr& filter, std::vector<party_ptr>& res)
{
	for(party_map::iterator i = parties_.begin(); i != parties_.end(); ++i) {
		if(!filter || filter->execute(*i->second).as_bool()) {
			res.push_back(i->second);
		}
	}
}
	hex::frustum view_volume;

void world::rebuild_drawing_caches(const std::set<hex::location>& visible) const
{
	hex::frustum::initialize();
	view_volume.set_volume_clip_space(-1, 1, -1, 1, -1, 1);
	tiles_.clear();
	current_loc_ = focus_->loc();
	std::cerr << "ZOOM: " << camera_.zoom() << "\n";
	std::cerr << "TILT: " << camera_.tilt() << "\n";
	int tiles_tried = 0;

	//find the initial ring which is within view
	hex::location hex_dir[6];
	for(int n = 0; n != 6; ++n) {
		hex_dir[n] = hex::tile_in_direction(current_loc_, static_cast<hex::DIRECTION>(n));
	}

	int core_radius = 1;
	bool done = false;
	while(!done) {
		for(int n = 0; n != 6; ++n) {
			hex_dir[n] = hex::tile_in_direction(hex_dir[n], static_cast<hex::DIRECTION>(n));
			if(!map_.is_loc_on_map(hex_dir[n]) || !view_volume.intersects(map_.get_tile(hex_dir[n]))) {
				done = true;
				break;
			}
		}

		++core_radius;
	}

	std::cerr << "core radius: " << core_radius << "\n";
	
	done = false;
	std::vector<hex::location> hexes;
	for(int radius = 0; !done; ++radius) {
		hexes.clear();
		hex::get_tile_ring(current_loc_, radius, hexes);
		tiles_tried += hexes.size();
		done = true;
		foreach(const hex::location& loc, hexes) {
			if(!map_.is_loc_on_map(loc)) {
				continue;
			}

			const hex::tile& t = map_.get_tile(loc);
			if(radius >= core_radius && !view_volume.intersects(t)) {
				//see if this tile has a cliff which is visible, in which case we should draw it.
				const hex::tile* cliffs[6];
				const int num_cliffs = t.neighbour_cliffs(cliffs);
				bool found = false;
				for(int n = 0; n != num_cliffs; ++n) {
					if(view_volume.intersects(*cliffs[n])) {
						found = true;
						break;
					}
				}

				if(!found) {
					continue;
				}
			} else {
				done = false;
			}
			tiles_.push_back(&t);
			tiles_.back()->load_texture();
		}
	}

	std::cerr << "TILES: " << tiles_.size() << "/" << tiles_tried << "\n";
	
	std::sort(tiles_.begin(),tiles_.end(), hex::tile::compare_texture());

	hex::tile::initialize_features_cache(
		&tiles_[0], &tiles_[0] + tiles_.size(), &features_cache_);

	track_info_grid_ = get_track_info();
}


void world::draw() const
{
	const std::set<hex::location>& visible =
		focus_->get_visible_locs();
	
	camera_controller_.prepare_selection();
	GLuint select_name = 0;
	for(party_map::const_iterator i = parties_.begin();
	    i != parties_.end(); ++i) {
		if(visible.count(i->second->loc())) {
			glLoadName(select_name);
			i->second->draw();
		}
		
		++select_name;
	}
	
	select_name = camera_controller_.finish_selection();
	hex::location selected_loc;
	party_map::const_iterator selected_party = parties_.end();
	if(select_name != GLuint(-1)) {
			party_map::const_iterator i = parties_.begin();
			std::advance(i, select_name);
			selected_loc = i->second->loc();
			selected_party = i;
	}
	
	if(focus_) {
		GLfloat buf[3];
		focus_->get_pos(buf);
		camera_.set_pan(buf);
	}
	
	camera_.prepare_frame();
	if(current_loc_ != focus_->loc() || camera_.moved_since_last_check()) {
		rebuild_drawing_caches(visible);
	}
	set_lighting();

	hex::tile::setup_drawing();
	foreach(const hex::tile* t, tiles_) {
		t->draw();
	}
	
	hex::tile::draw_features(&tiles_[0], &tiles_[0] + tiles_.size(),
				 features_cache_);
	hex::tile::finish_drawing();

	foreach(const hex::tile* t, tiles_) {
		t->draw_cliffs();
	}
	foreach(const hex::tile* t, tiles_) {
		t->emit_particles(particle_system_);
	}

	if(show_grid_) {
		glDisable(GL_LIGHTING);
		foreach(const hex::tile* t, tiles_) {
			t->draw_grid();
		}
		glEnable(GL_LIGHTING);
	}

	if(selected_party != parties_.end()) {
		std::vector<const hex::tile*> tiles;
		hex::line_of_sight(map_,focus_->loc(),selected_party->second->loc(),&tiles);
		foreach(const hex::tile* t, tiles) {
			t->draw_highlight();
		}
	}
	
	if(map_.is_loc_on_map(selected_loc)) {
		glDisable(GL_LIGHTING);
		map_.get_tile(selected_loc).draw_highlight();
		glEnable(GL_LIGHTING);
	}
	
	for(party_map::const_iterator i = parties_.begin();
	    i != parties_.end(); ++i) {
		if(visible.count(i->second->loc())) {
			i->second->draw();
		}
	}
	
	std::vector<const_settlement_ptr> seen_settlements;
	for(settlement_map::const_iterator i =
		    settlements_.begin(); i != settlements_.end(); ++i) {
		if(visible.count(i->first) &&
		   !std::count(seen_settlements.begin(), seen_settlements.end(), i->second)) {
			i->second->draw();
			seen_settlements.push_back(i->second);
		}
	}

	particle_system_.draw();
	
	const SDL_Color white = {0xFF,0xFF,0x0,0};
	
	
	const graphics::texture text = graphics::font::render_text(fps_track_.msg() + (formatter() << " " << tiles_.size()).str(),20,white); graphics::prepare_raster();
	graphics::blit_texture(text,50,50);
	
	if(track_info_grid_) {
		track_info_grid_->draw();
	}
	
	if(focus_) {
		const hex::location& loc = focus_->loc();
		if(map().is_loc_on_map(loc)) {
			const hex::tile& t = map_.get_tile(loc);
			const int height = t.height();
			std::ostringstream stream;
			stream << height << " elevation";
			const graphics::texture text =
				graphics::font::render_text(stream.str(),20,white);
			graphics::blit_texture(text,50,80);
			
			graphics::texture fatigue_text = graphics::font::render_text(focus_->fatigue_status_text(), 20, white);
			graphics::blit_texture(fatigue_text,50,140);
		}
	}
	
	{
		const int hour = current_time().hour();
		const int min = current_time().minute();
		std::ostringstream stream;
		stream << (hour < 10 ? "0" : "") << hour << ":"
		       << (min < 10 ? "0" : "") << min;
		const graphics::texture text = graphics::font::render_text(stream.str(),20,white);
		graphics::blit_texture(text,50,110);
	}
	
	blit_texture(compass_, 1024-compass_.height(),0,-camera_.current_rotation());
	
	if(selected_party != parties_.end()) {
		graphics::texture text = graphics::font::render_text(selected_party->second->status_text(), 20, white);
		graphics::blit_texture(text,1024 - 50 - text.width(),200);
	}
}

void world::play()
{
	world_context context(this);
	if(!get_pc_party()) {
		for(settlement_map::iterator i = settlements_.begin();
		    i != settlements_.end(); ++i) {
			if(party_ptr p = i->second->get_world().get_pc_party()) {
				i->second->play();
				time_ = i->second->get_world().current_time();
				p->new_world(*this,p->loc());
				add_party(p);
				break;
			}
		}
	}

	party_ptr active_party;

	const GLfloat game_speed = 0.2;

	graphics::frame_skipper skippy(50, preference_maxfps());
	fps_track_.reset();

	bool quit = false;

	for(bool done = false; !done; ) {
		if(!focus_) {
			for(party_map::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
				if(i->second->is_human_controlled()) {
					focus_ = i->second;
					break;
				}
			}

			if(!focus_) {
				break;
			}
		}

		{
			bool draw_this_frame = !skippy.skip_frame();
			
			if(draw_this_frame) {
				draw();
				if(game_bar_) {
					game_bar_->draw();
				}
				SDL_GL_SwapBuffers();
			}
			fps_track_.register_frame(draw_this_frame);
		}
		
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(game_bar_) {
				const int start = SDL_GetTicks();
				bool preclaimed = game_bar_->process_event(event);
				const int end = SDL_GetTicks();
				if(end - start > 20) {
					/* more than 20ms processing an event ... */
					skippy.reset();
					fps_track_.reset();
				}
				if(preclaimed) continue;
			}
			switch(event.type) {
				case SDL_QUIT:
					done = true;
					quit = true;
					break;
				default:
					break;
			}
		}

		if(!active_party) {
			active_party = get_party_ready_to_move();
		}
		party::TURN_RESULT party_result = party::TURN_COMPLETE;
		while(active_party && party_result == party::TURN_COMPLETE) {
			party_map::iterator itor = find_party(active_party);
			hex::location start_loc = active_party->loc();
			party_result = active_party->play_turn();
			if(party_result != party::TURN_STILL_THINKING) {
				if(itor != parties_.end()) {
					parties_.erase(itor);
				}

 				if(start_loc != active_party->loc()) {
					party_map_range range = parties_.equal_range(
						active_party->loc());
					bool path_cleared = true;
					bool were_encounters = false;

					while(range.first != range.second && !active_party->is_destroyed()) {
						if(active_party->is_human_controlled() || 
						   range.first->second->is_human_controlled()) {
							were_encounters = true;
						}
						handle_encounter(active_party,range.first->second, map());
						if(range.first->second->is_destroyed()) {
							parties_.erase(range.first++);
						} else {
							path_cleared = false;
							++range.first;
						}
					}
					if(!path_cleared) {
						active_party->set_loc(start_loc);
					}
					if(were_encounters) {
						skippy.reset();
						fps_track_.reset();
					}
				}

				if(active_party->is_destroyed() == false) {
					std::map<hex::location,hex::location>::const_iterator exit = exits_.find(active_party->loc());
					if(exit != exits_.end() && active_party->is_human_controlled()) {
						active_party->set_loc(exit->second);
						remove_party(active_party);
						std::cerr << "returning from world...\n";
						return;
					}

					settlement_map::iterator s = settlements_.find(active_party->loc());
					if(s != settlements_.end() && active_party->is_human_controlled()) {
						remove_party(active_party);
						//enter the new world
						time_ = s->second->enter(active_party, active_party->loc(), time_);

						//player has left the settlement, return to this world
						active_party->new_world(*this,active_party->loc());
						skippy.reset();
						fps_track_.reset();
					}

					if(active_party->is_destroyed() == false) {
						parties_.insert(std::pair<hex::location,party_ptr>(
						                   active_party->loc(),active_party));
						queue_.push(active_party);
					}
				}

				active_party = get_party_ready_to_move();
				if(active_party && active_party->is_human_controlled()) {
					focus_ = active_party;
				}
			}
		}

		const Uint8* keys = SDL_GetKeyState(NULL);

		if(!active_party) {
			subtime_ += game_speed * (keys[SDLK_SPACE] ? 2.0 : 1.0);
			if(subtime_ >= 1.0) {
				++time_;
				subtime_ = 0.0;
			}
		}

		if(keys[SDLK_g]) {
			show_grid_ = true;
		}

		if(keys[SDLK_h]) {
			show_grid_ = false;
		}

		camera_controller_.keyboard_control();
	}
	if(quit) {
		SDL_Event e;
		e.type = SDL_QUIT;
		SDL_PushEvent(&e);
	}
}

world::party_map::iterator world::find_party(const_party_ptr p)
{
	for(party_map_range range = parties_.equal_range(p->loc());
	    range.first != range.second; ++range.first) {
		if(range.first->second == p) {
			return range.first;
		}
	}
	
	return parties_.end();
}

party_ptr world::get_party_ready_to_move()
{
	while(!queue_.empty() && queue_.top()->ready_to_move_at() <= time_){
		party_ptr res = queue_.top();
		queue_.pop();
		if(find_party(res) != parties_.end()) {
			return res;
		}
	}

	return party_ptr();
}

gui::const_grid_ptr world::get_track_info() const
{
	if(!focus_) {
		return gui::const_grid_ptr();
	}

	const int track = focus_->track();
	std::cerr << "track ability: " << track << "\n";

	const tracks::tracks_list& list = tracks_.get_tracks(focus_->loc(),time_);
	if(list.empty()) {
		return gui::const_grid_ptr();
	}

	gui::grid_ptr g(new gui::grid(2));

	foreach(const tracks::info& info, list) {
		if(info.party_id == focus_->id()) {
			continue;
		}

		std::cerr << "visible: " << (info.visibility*track) << "\n";
		if(info.visibility*track < 1000) {
			continue;
		}

		static const std::string image_name = "arrow.png";
		gui::image_widget_ptr img(new gui::image_widget(image_name,50,50));

		hex::DIRECTION dir = info.dir;
		switch(dir) {
		case hex::NORTH_EAST:
			dir = hex::NORTH_WEST;
			break;
		case hex::SOUTH_EAST:
			dir = hex::SOUTH_WEST;
			break;
		case hex::NORTH_WEST:
			dir = hex::NORTH_EAST;
			break;
		case hex::SOUTH_WEST:
			dir = hex::SOUTH_EAST;
			break;
		}
		
		const int age = time_ - info.time;
		std::string msg;
		if(age <= 60) {
			msg = "tracks_hour_or_less";
		} else if(age <= 300) {
			msg = "tracks_few_hours";
		} else {
			msg = "tracks_hours";
		}
		img->set_rotation((int(dir)+3)*60.0 - camera_.current_rotation());
		g->add_col(img)
		  .add_col(gui::label::create(msg, graphics::color_yellow(), 18));
	}

	g->set_loc(20, 300);
	g->set_show_background(false);

	return g;
}

void world::set_lighting() const
{
	if(time_.hour() >= 6 && time_.hour() <= 17) {
		const GLfloat x = 2.0*GLfloat((time_.hour()-6)*60 + time_.minute())/(60.0*12.0) - 1.0;
		const GLfloat y = -std::sqrt(std::max(0.0,1.0 - x*x));

		GLfloat direction[] = {x,0.0,y,0.0};
		glLightfv(GL_LIGHT0, GL_POSITION, direction);
	} else {
		GLfloat direction[] = {0.0,0.0,0.0,0.0};
		glLightfv(GL_LIGHT0, GL_POSITION, direction);
	}

	GLfloat ambient[] = {1.0,1.0,1.0,1.0};
	GLfloat diffuse[] = {1.0,1.0,1.0,1.0};

	if(sun_light_) {
		const int res = sun_light_->execute(time_).as_int();
		diffuse[0] = GLfloat((res/10000)%100)/100.0;
		diffuse[1] = GLfloat((res/100)%100)/100.0;
		diffuse[2] = GLfloat(res%100)/100.0;
	}

	if(ambient_light_) {
		const int res = ambient_light_->execute(time_).as_int();
		ambient[0] = GLfloat((res/10000)%100)/100.0;
		ambient[1] = GLfloat((res/100)%100)/100.0;
		ambient[2] = GLfloat(res%100)/100.0;
	}

	camera_.set_background_color(ambient);

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}

void world::fire_event(const std::string& name, const formula_callable& info)
{
	//std::cerr << "event: '" << name << ": " << handlers_.size() << "'\n";
	std::pair<event_map::iterator,event_map::iterator> range =
	     handlers_.equal_range(name);
	while(range.first != range.second) {
		std::cerr << "calling handler...\n";
		range.first->second.handle(info, *this);
		++range.first;
	}
}

void world::add_event_handler(const std::string& event, const event_handler& handler)
{
	handlers_.insert(std::pair<std::string,event_handler>(event,handler));
}

party_ptr world::get_pc_party() const
{
	for(party_map::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
		if(i->second->is_human_controlled()) {
			return i->second;
		}
	}

	return party_ptr();
}

bool world::remove_party(party_ptr p)
{
	std::pair<party_map::iterator,party_map::iterator> range = parties_.equal_range(p->loc());
	while(range.first != range.second) {
		if(range.first->second == p) {
			parties_.erase(range.first);
			return true;
		}

		++range.first;
	}

	return false;
}

}
