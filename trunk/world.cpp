
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "character_status_dialog.hpp"
#include "encounter.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "settlement.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "wml_utils.hpp"
#include "world.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

namespace game_logic
{

world::world(wml::const_node_ptr node)
  : map_(sys::read_file((*node)["map"])), camera_(map_),
    time_(node), subtime_(0.0), tracks_(map_)
{
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
		settlement_ptr s(new settlement(s1->second,map_));
		settlements_[s->loc()] = s;
	}

	wml::node::const_child_iterator e1 = node->begin_child("exit");
	const wml::node::const_child_iterator e2 = node->end_child("exit");
	for(; e1 != e2; ++e1) {
		hex::location loc1(wml::get_attr<int>(e1->second,"x"),
		                   wml::get_attr<int>(e1->second,"y"));
		hex::location loc2(wml::get_attr<int>(e1->second,"xdst"),
		                   wml::get_attr<int>(e1->second,"ydst"));
		exits_[loc1] = loc2;
	}
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

void world::add_party(party_ptr new_party)
{
	parties_.insert(std::pair<hex::location,party_ptr>(
	                 new_party->loc(),new_party));
	queue_.push(new_party);

	std::cerr << "added party at " << new_party->loc().x() << "," << new_party->loc().y() << "\n";
	if(new_party->is_human_controlled()) {
		std::cerr << "is human\n";
		focus_ = new_party;
	}
}

void world::play()
{
	party_ptr active_party;

	std::string fps_msg;
	int last_fps_frames = 0;
	int last_fps_ticks = 0;

	const int time_between_frames = 20;
	const GLfloat game_speed = 0.2;
	int last_draw = -1;

	graphics::texture compass(graphics::texture::get(graphics::surface_cache::get("compass-rose.png")));
	bool show_grid = true;
	int frame_num = 0;
	hex::location current_loc;
	std::vector<const hex::tile*> visible_tiles, dark_tiles;
	hex::tile::features_cache visible_features_cache, dark_features_cache;
	gui::const_grid_ptr track_info_grid;
	for(bool done = false; !done; ++frame_num) {
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

		const std::set<hex::location>& visible =
		     focus_->get_visible_locs();

		bool recalculate_tiles = current_loc != focus_->loc();
		if(recalculate_tiles) {
			visible_tiles.clear();
			dark_tiles.clear();
			current_loc = focus_->loc();
			std::vector<hex::location> hexes;
			for(int n = 0; n != 30; ++n) {
				hex::get_tile_ring(focus_->loc(), n, hexes);
			}

			foreach(const hex::location& loc, hexes) {
				if(!map_.is_loc_on_map(loc)) {
					continue;
				}

				if(visible.count(loc)) {
					visible_tiles.push_back(&map_.get_tile(loc));
				} else {
					dark_tiles.push_back(&map_.get_tile(loc));
				}
			}

			std::sort(visible_tiles.begin(),visible_tiles.end(),
			          hex::tile::compare_texture());
			std::sort(dark_tiles.begin(),dark_tiles.end(),
			          hex::tile::compare_texture());

			hex::tile::initialize_features_cache(
			    &visible_tiles[0], &visible_tiles[0] + visible_tiles.size(),
			    &visible_features_cache);
			hex::tile::initialize_features_cache(
			    &dark_tiles[0], &dark_tiles[0] + dark_tiles.size(),
			    &dark_features_cache);

			track_info_grid = get_track_info();
		}

		camera_.prepare_selection();
		GLuint select_name = 0;
		for(party_map::iterator i = parties_.begin();
		    i != parties_.end(); ++i) {
			if(visible.count(i->second->loc())) {
				glLoadName(select_name);
				i->second->draw();
			}

			++select_name;
		}

		select_name = camera_.finish_selection();
		hex::location selected_loc;
		party_map::iterator selected_party = parties_.end();
		if(select_name != GLuint(-1)) {
			party_map::iterator i = parties_.begin();
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
		set_lighting();

		hex::tile::setup_drawing();
		foreach(const hex::tile* t, dark_tiles) {
			t->draw();
		}

		hex::tile::draw_features(&dark_tiles[0],
		                         &dark_tiles[0] + dark_tiles.size(),
		                         dark_features_cache);
	
		foreach(const hex::tile* t, visible_tiles) {
			t->draw();
		}

		hex::tile::draw_features(&visible_tiles[0],
		                         &visible_tiles[0] + visible_tiles.size(),
		                         visible_features_cache);
		hex::tile::finish_drawing();

		foreach(const hex::tile* t, visible_tiles) {
			t->draw_cliffs();
		}

		foreach(const hex::tile* t, dark_tiles) {
			t->draw_cliffs();
		}

		if(show_grid) {
			glDisable(GL_LIGHTING);
			foreach(const hex::tile* t, visible_tiles) {
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

		for(party_map::iterator i = parties_.begin();
		    i != parties_.end(); ++i) {
			if(visible.count(i->second->loc())) {
				i->second->draw();
			}
		}

		for(settlement_map::const_iterator i =
		    settlements_.begin(); i != settlements_.end(); ++i) {
			if(visible.count(i->second->loc())) {
				i->second->draw();
			}
		}

		const SDL_Color white = {0xFF,0xFF,0x0,0};
		const GLfloat seconds = static_cast<GLfloat>(SDL_GetTicks() - last_fps_ticks)/1000.0;
		if(seconds > 1.0) {
			const int frames = frame_num - last_fps_frames;
			int fps = static_cast<int>(
			    static_cast<GLfloat>(frames)/seconds);
			std::ostringstream stream;
			stream << fps << "fps";
			fps_msg = stream.str();
			last_fps_frames = frame_num;
			last_fps_ticks = SDL_GetTicks();
		}

		const graphics::texture text =
		         graphics::font::render_text(fps_msg,20,white);
		graphics::prepare_raster();
		graphics::blit_texture(text,50,50);

		if(track_info_grid) {
			track_info_grid->draw();
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

		blit_texture(compass, 1024-compass.height(),0,-camera_.current_rotation());

		if(selected_party != parties_.end()) {
			graphics::texture text = graphics::font::render_text(selected_party->second->status_text(), 20, white);
			graphics::blit_texture(text,1024 - 50 - text.width(),200);
		}

		SDL_GL_SwapBuffers();

		if(last_draw != -1) {
			const int ticks = SDL_GetTicks();
			const int target = last_draw + time_between_frames;
			if(ticks >= target) {
				SDL_Delay(1);
			} else {
				SDL_Delay(target - ticks);
			}
		}

		last_draw = SDL_GetTicks();

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					done = true;
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
			party_result = active_party->play_turn();
			if(party_result != party::TURN_STILL_THINKING) {
				if(itor != parties_.end()) {
					parties_.erase(itor);
				}

				party_map_range range = parties_.equal_range(
				                active_party->loc());
				while(range.first != range.second && !active_party->is_destroyed()) {
					handle_encounter(active_party,range.first->second,
					                 map());
					if(range.first->second->is_destroyed()) {
						parties_.erase(range.first++);
					} else {
						++range.first;
					}
				}

				if(active_party->is_destroyed() == false) {
					std::map<hex::location,hex::location>::const_iterator exit = exits_.find(active_party->loc());
					if(exit != exits_.end() && active_party->is_human_controlled()) {
						active_party->set_loc(exit->second);
						std::cerr << "returning from world...\n";
						return;
					}

					settlement_map::iterator s = settlements_.find(active_party->loc());
					if(s != settlements_.end()) {
						s->second->enter(active_party);
						active_party->new_world(*this,active_party->loc());
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

		if(keys[SDLK_ESCAPE]) {
			done = true;
		}

		if(keys[SDLK_g]) {
			show_grid = true;
		}

		if(keys[SDLK_h]) {
			show_grid = false;
		}

		if(keys[SDLK_s]) {
			game_dialogs::character_status_dialog(focus_->members().front(), focus_).show_modal();
		}

		camera_.keyboard_control();
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

	const tracks::tracks_list& list = tracks_.get_tracks(focus_->loc(),time_);
	if(list.empty()) {
		return gui::const_grid_ptr();
	}

	gui::grid_ptr g(new gui::grid(2));

	foreach(const tracks::info& info, list) {
		if(info.party_id == focus_->id()) {
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
		const int res = sun_light_->execute(time_);
		diffuse[0] = GLfloat((res/10000)%100)/100.0;
		diffuse[1] = GLfloat((res/100)%100)/100.0;
		diffuse[2] = GLfloat(res%100)/100.0;
	}

	if(ambient_light_) {
		const int res = ambient_light_->execute(time_);
		ambient[0] = GLfloat((res/10000)%100)/100.0;
		ambient[1] = GLfloat((res/100)%100)/100.0;
		ambient[2] = GLfloat(res%100)/100.0;
	}

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
}

}
