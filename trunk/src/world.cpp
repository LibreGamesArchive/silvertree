
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "encounter.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "frustum.hpp"
#include "global_game_state.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "keyboard.hpp"
#include "label.hpp"
#include "party_status_dialog.hpp"
#include "pathfind.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "settlement.hpp"
#include "surface.hpp"
#include "surface_cache.hpp"
#include "text.hpp"
#include "wml_parser.hpp"
#include "wml_utils.hpp"
#include "world.hpp"

#include <cmath>
#include <iostream>
#include <sstream>

#include "audio/openal.hpp"

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
      scale_(wml::get_int(node, "scale", 1)),
      time_(node), subtime_(0.0), tracks_(map_),
      border_tile_(wml::get_str(node, "border_tile")),
      done_(false), quit_(false),
      input_listener_(this), 
      renderer_(map_, camera_),
      selection_(renderer_)
{
    const std::string& sun_light = wml::get_str(node, "sun_light");
    if(!sun_light.empty()) {
        sun_light_.reset(new formula(sun_light));
    }
    
    const std::string& ambient_light = wml::get_str(node, "ambient_light");
    if(!ambient_light.empty()) {
        ambient_light_.reset(new formula(ambient_light));
    }
    
    const std::string& party_light = wml::get_str(node, "party_light");
    if(!party_light.empty()) {
        party_light_.reset(new formula(party_light));
    }
    
    const std::string& party_light_power = wml::get_str(node, "party_light_power");
    if(!party_light_power.empty()) {
        party_light_power_.reset(new formula(party_light_power));
    }
    
    music_file_ = wml::get_str(node, "music");
    
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
        hex::location loc1(wml::get_attr<int>(e1->second,"x", -1),
                           wml::get_attr<int>(e1->second,"y", -1));
        hex::location loc2(wml::get_attr<int>(e1->second,"xdst",-1),
                           wml::get_attr<int>(e1->second,"ydst",-1));
        
        const std::string& form = e1->second->attr("formula");
        if(!form.empty()) {
            const int begin = SDL_GetTicks();
            formula f(form);
            hex::location_ptr loc_ptr(new hex::location);
            for(int x = 0; x != map_.size().x(); ++x) {
                for(int y = 0; y != map_.size().y(); ++y) {
                    *loc_ptr = hex::location(x,y);
                    if(f.execute(*loc_ptr).as_bool()) {
                        exits_[*loc_ptr] = loc2;
                    }
                }
            }
            
            const int time = SDL_GetTicks() - begin;
            std::cerr << "time: " << time << "\n";
        }
        
        if(loc1.valid()) {
            exits_[loc1] = loc2;
        }
    }
    
    const std::vector<wml::const_node_ptr> portals = wml::child_nodes(node, "portal");
    foreach(const wml::const_node_ptr& portal, portals) {
        hex::location loc1(wml::get_attr<int>(portal,"xdst"),
                           wml::get_attr<int>(portal,"ydst"));
        hex::location loc2(wml::get_attr<int>(portal,"xsrc"),
                           wml::get_attr<int>(portal,"ysrc"));
        exits_[loc1] = loc2;
    }
    
    const std::vector<wml::const_node_ptr> events = wml::child_nodes(node, "event");
    foreach(const wml::const_node_ptr& event, events) {
        event_handler handler(event);
        const std::string& name = event->attr("event");
        add_event_handler(name, handler);
    }
    
    wml::const_node_ptr tracks = node->get_child("tracks");
    if(tracks) {
        tracks_.read(tracks);
    }
    
    keys_.bind_key(ACCEL_TIME_KEY, SDLK_SPACE, KMOD_NONE);
    keys_.bind_key(SUPER_ACCEL_TIME_KEY, SDLK_SPACE, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(SHOW_GRID, SDLK_g, KMOD_NONE);
    keys_.bind_key(HIDE_GRID, SDLK_h, KMOD_NONE);

    find_focus();
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

	if(party_light_) {
		res->set_attr("party_light", party_light_->str());
	}

	if(party_light_power_) {
		res->set_attr("party_light_power", party_light_power_->str());
	}

    if(!music_file_.empty()) {
        res->set_attr("music", music_file_);
    }

	res->set_attr("border_tile", border_tile_);

	for(party_map::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
		res->add_child(i->second->write());
	}

	for(std::map<hex::location,hex::location>::const_iterator i = exits_.begin(); i != exits_.end(); ++i) {
		wml::node_ptr portal(new wml::node("exit"));
		portal->set_attr("x", boost::lexical_cast<std::string>(i->first.x()));
		portal->set_attr("y", boost::lexical_cast<std::string>(i->first.y()));
		portal->set_attr("xdst", boost::lexical_cast<std::string>(i->second.x()));
		portal->set_attr("ydst", boost::lexical_cast<std::string>(i->second.y()));
		res->add_child(portal);
	}

	for(settlement_map::const_iterator i = settlements_.begin(); i != settlements_.end(); ++i) {
		res->add_child(i->second->write());
	}

	for(event_map::const_iterator i = handlers_.begin(); i != handlers_.end(); ++i) {
		wml::const_node_ptr event = i->second.write();
		if(event) {
			wml::node_ptr event_copy = wml::deep_copy(event);
			event_copy->set_attr("event", i->first);
			res->add_child(event_copy);
		}
	}

	res->add_child(tracks_.write());

	return res;
}

game_logic::const_party_ptr world::get_party_at(const hex::location& loc) const
{
	const_party_map_range range = parties_.equal_range(loc);
	if(range.first != range.second) {
		return range.first->second;
	}
	
	return game_logic::const_party_ptr();
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
							   graphics::screen_width(), 128, new_party, this));
		game_bar_->set_frame(gui::frame_manager::make_frame(game_bar_, "game-bar-frame"));
	}

	return res;
}

void world::relocate_party(party_ptr party, const hex::location& loc)
{
	if(remove_party(party)) {
		parties_.insert(std::pair<hex::location,party_ptr>(loc,party));
	}
	party->set_loc(loc);
}

void world::get_matching_parties(const formula* filter, std::vector<party_ptr>& res)
{
	for(party_map::iterator i = parties_.begin(); i != parties_.end(); ++i) {
		if(!filter || filter->execute(*i->second).as_bool()) {
			res.push_back(i->second);
		}
	}
}

bool world::draw() const
{

    // temporary fix
    if(!focus_) {
        return false;
    }

    renderer_.reset_state();

    assert(focus_);
    const std::set<hex::location>& visible =
        focus_->get_visible_locs();
    
    hex::location selected_loc = selection_.get_selected_hex();
    std::vector<hex::location> path;
    const_party_ptr selected_party = get_party_at(selected_loc);
    if(map_.is_loc_on_map(selected_loc)) {
        const bool adjacent_only = visible.count(selected_loc) && parties_.count(selected_loc);
        hex::find_path(focus_->loc(), selected_loc, *focus_, &path, 500, adjacent_only);
    }
    
    if(path.empty() && focus_->get_current_path()) {
        path = *focus_->get_current_path();
    }
    renderer_.set_path(path);
    
    GLfloat pan_buf[3];
    focus_->get_pos(pan_buf);
    renderer_.set_pos(pan_buf, focus_->loc());
    
    if(selected_party) {
        std::vector<hex::location> locs;
        map_.has_line_of_sight(focus_->loc(),selected_party->loc(),&locs);
        renderer_.add_highlights(locs);
    }
    
    if(map_.is_loc_on_map(selected_loc)) {
        renderer_.add_highlight(selected_loc);
    }
    
    std::vector<const_party_ptr> enemies;
    for(party_map::const_iterator i = parties_.begin();
        i != parties_.end(); ++i) {
        if(visible.count(i->second->loc())) {
            renderer_.add_avatar(i->second->avatar());
            if(focus_ && i->second->is_enemy(*focus_)) {
                enemies.push_back(i->second);
            }
        }
    }
    
    std::vector<const_settlement_ptr> seen_settlements;
    for(settlement_map::const_iterator i =
            settlements_.begin(); i != settlements_.end(); ++i) {
        if(visible.count(i->first) &&
           !std::count(seen_settlements.begin(), seen_settlements.end(), i->second)) {
            foreach(hex::const_map_avatar_ptr avatar, i->second->avatars()) {
                renderer_.add_avatar(avatar);
            }
            seen_settlements.push_back(i->second);
        }
    }
    
    if(!enemies.empty()) {
        foreach(const const_party_ptr& p, enemies) {
            GLfloat from[3];
            GLfloat to[3];
            const static GLfloat color[4] = { 1.0, 0.0, 0.0, 1.0 };
            focus_->get_pos(from);
            p->get_pos(to);
            from[2] += 0.5;
            to[2] += 0.5;
            renderer_.add_sight_line(from, to, color);
        }
    }
    
    set_lighting(renderer_);
    bool drew = renderer_.draw();
    if(drew) {
        draw_display(selected_party);
    }
    
#ifdef AUDIO
    if(audio::audio_available() && audio_) {
        audio_->pump_sound();
    }
#endif

    return drew;
}

void world::draw_display(const_party_ptr selected_party) const {
    const SDL_Color white = {0xFF,0xFF,0x0,0};

    graphics::prepare_raster();
    text::renderer& text_renderer = text::renderer::instance();
    
    const text::rendered_text_ptr text = 
        text_renderer.render(renderer_.status_text(),20,white); 
    text->blit(50,10);
    
    track_info_grid_ = get_track_info();
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
            const text::rendered_text_ptr text =
                text_renderer.render(stream.str(),20,white);
            text->blit(50,50);
            
            text::rendered_text_ptr fatigue_text = 
                text_renderer.render(focus_->fatigue_status_text(), 20, white);
            fatigue_text->blit(50,140);
        }
    }
    
    {
        const int hour = current_time().hour();
        const int min = current_time().minute();
        std::ostringstream stream;
        stream << (hour < 10 ? "0" : "") << hour << ":"
               << (min < 10 ? "0" : "") << min;
        text::rendered_text_ptr text = text_renderer.render(stream.str(),20,white);
        text->blit(50,110);
    }
    
    blit_texture(compass_, 1024-compass_.height(),0,-camera_.current_rotation());
    
    if(selected_party) {
        text::rendered_text_ptr text = text_renderer.render(selected_party->status_text(), 20, white);
        text->blit(1024 - 50 - text->width(),200);
    }
    
    if(!chat_labels_.empty()) {
        const int ticks = SDL_GetTicks();
        for(std::vector<chat_label>::iterator cl = chat_labels_.begin(); cl != chat_labels_.end(); ) {
            const int age = ticks - cl->started_at;
            if(age > 10000) {
                cl = chat_labels_.erase(cl);
            } else {
                if(age >= 0) {
                    const int color = cl->character->color().as_int();
                    GLfloat colorv[4] = {
                        GLfloat((color/10000)%100)/100.0,
                        GLfloat((color/100)%100)/100.0,
                        GLfloat(color%100)/100.0,
                        1.0 - pow(static_cast<GLfloat>(age)/10000.0, 2.0)
                    };
                    
                    glColor4fv(colorv);
                    cl->label->set_loc(cl->label->x(), cl->y_loc - age/100);
                    cl->label->draw();
                }
                ++cl;
            }
        }
    }
    
    glColor4f(1.0,1.0,1.0,1.0);
    if(game_bar_) {
        game_bar_->draw();
    }
}

namespace {
const GLfloat game_speed = 0.2;
}

void world::find_focus() {
    for(party_map::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
        if(i->second->is_human_controlled()) {
            focus_ = i->second;
            break;
        }
    }
}

void world::play()
{
    world_context context(this);
#ifdef AUDIO
    audio::scoped_audio_context_ptr my_audio(audio_, new audio::audio_context());
    audio::source_ptr source;
    audio::stream_ptr music;
#endif
    if(!get_pc_party()) {
        for(settlement_map::iterator i = settlements_.begin();
            i != settlements_.end(); ++i) {
            if(party_ptr p = i->second->get_world().get_pc_party()) {
                i->second->play();
                time_ = i->second->get_world().current_time();
                p->new_world(*this,p->loc(),p->last_move());
                add_party(p);
                break;
            }
        }
    }

#ifdef AUDIO
    if(audio::audio_available() && !music_file_.empty()) {
        source = my_audio->make_source();
        music = my_audio->make_stream(sys::find_file(music_file_));
        music->set_looping(true);
        source->set_sound(music.get());
        source->play();
    }
#endif
      
    map_formula_callable_ptr standard_callable(new map_formula_callable);
    standard_callable->add("world", variant(this))
        .add("pc", variant(get_pc_party().get()))
        .add("var", variant(&global_game_state::get().get_variables()));
    
    fire_event("start", *standard_callable);
    
    party_ptr active_party;
    
    renderer_.reset_timing();
    
    input::pump input_pump;
    input_pump.register_listener(game_bar_);
    input_pump.register_listener(&keyboard::global_controls);
    input_pump.register_listener(&keys_);
    input_pump.register_listener(&input_listener_);
    input_pump.register_listener(&camera_controller_);
    input_pump.register_listener(&selection_);

    while(!done_) {
        if(!focus_) {
            find_focus();
            if(!focus_) {
                break;
            }
        }
        
        draw();
        SDL_GL_SwapBuffers();
        
        if(!input_pump.process()) {
            done_ = true;
            quit_ = true;
        }                 
        
        if(!script_.empty()) {
            bool scripted_moves = false;
            for(party_map::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
                if(i->second->has_script()) {
                    scripted_moves = true;
                    break;
                }
            }
            
            if(!scripted_moves) {
                map_formula_callable_ptr script_callable(new map_formula_callable);
                script_callable->add("script", variant(script_))
                    .add("pc", variant(get_pc_party().get()))
                    .add("world", variant(this))
                    .add("var", variant(&global_game_state::get().get_variables()));
                script_.clear();
                fire_event("finish_script", *script_callable);
                renderer_.reset_timing();
            }
        }
        
        if(!active_party) {
            active_party = get_party_ready_to_move();
        }
        
        party::TURN_RESULT party_result = party::TURN_COMPLETE;
        while(active_party && party_result == party::TURN_COMPLETE) {
            if(active_party->loc() != active_party->previous_loc()) {
                active_party->finish_move();
            }
            
            hex::location start_loc = active_party->loc();
            
            if(script_.empty() || active_party->has_script()) {
                party_result = active_party->play_turn();
            } else {
                active_party->pass();
            }
            
            if(party_result != party::TURN_STILL_THINKING) {
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
                        renderer_.reset_timing();
                    }
                }
                
                //find the party in the map and erase it.
                std::pair<party_map::iterator,party_map::iterator> loc_range = parties_.equal_range(start_loc);
                while(loc_range.first != loc_range.second) {
                    if(loc_range.first->second == active_party) {
                        parties_.erase(loc_range.first);
                        break;
                    }
                    ++loc_range.first;
                }
                
                if(active_party->is_destroyed() == false) {
                    std::map<hex::location,hex::location>::const_iterator exit = exits_.find(active_party->loc());
                    if(script_.empty() && exit != exits_.end() && active_party->is_human_controlled()) {
                        std::cerr << "exiting through exit at " << active_party->loc().x() << "," << active_party->loc().y() << "\n";
                        active_party->set_loc(exit->second);
                        remove_party(active_party);
                        std::cerr << "returning from world...\n";
                        return;
                    }
                    
                    settlement_map::iterator s = settlements_.find(active_party->loc());
                    if(s != settlements_.end() && active_party->is_human_controlled()) {
                        remove_party(active_party);
                        //enter the new world
                        time_ = s->second->enter(active_party, active_party->loc(), time_, *this);
                        
                        //player has left the settlement, return to this world
                        active_party->new_world(*this,active_party->loc(),active_party->last_move());
						camera_.set_rotation(s->second->get_world().camera_);
                        renderer_.reset_timing();
					    map_formula_callable_ptr standard_callable(new map_formula_callable);
						//since we've left the settlement we fire a 'start'
						//event, since we're in this world again.
					    standard_callable->add("world", variant(this))
				            .add("pc", variant(get_pc_party().get()))
					        .add("var", variant(&global_game_state::get().get_variables()));
    					fire_event("start", *standard_callable);
                    }
                    
                    if(active_party->is_destroyed() == false) {
                        parties_.insert(std::pair<hex::location,party_ptr>(active_party->loc(),active_party));
                        queue_.push(active_party);
                    }
                }
                
                active_party = get_party_ready_to_move();
                if(!focus_ && active_party && active_party->is_human_controlled()) {
                    focus_ = active_party;
                }
            }
        }
        
        if(!active_party) {
            double increase = scale_ * game_speed;

            if(keys_.key(SUPER_ACCEL_TIME_KEY)) {
                increase *= 4;
            } else if(keys_.key(ACCEL_TIME_KEY) || !script_.empty()) {
                increase *= 2;
            } 
            
            subtime_ += increase;
            
            if(subtime_ >= 1.0) {
                time_ += static_cast<int>(subtime_);
                subtime_ = 0.0;
                fire_event("tick", *standard_callable);
            }
        }
        
        if(keys_.key(SHOW_GRID)) {
            renderer_.set_show_grid(true);
        } else if(keys_.key(HIDE_GRID)) {
            renderer_.set_show_grid(false);
        }
        
        camera_controller_.update();
    }
    if(quit_) {
        throw quit_exception();
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

	const tracks::tracks_list& list = tracks_.get_tracks(focus_->loc(),time_);
	if(list.empty()) {
		return gui::const_grid_ptr();
	}

	gui::grid_ptr g(new gui::grid(2));

	foreach(const tracks::info& info, list) {
		if(info.party_id == focus_->id()) {
			continue;
		}

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

void world::set_lighting(graphics::renderer& renderer) const {
    GLfloat direction[4] = {0.0,0.0,0.0,0.0};

    if(time_.hour() >= 6 && time_.hour() <= 17) {
        const GLfloat x = 2.0*GLfloat((time_.hour()-6)*60 + time_.minute())/(60.0*12.0) - 1.0;
        const GLfloat y = std::sqrt(std::max(0.0,1.0 - x*x));
        
        direction[0] = x;
        direction[2] = y;
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
    
    renderer.set_sky_color(ambient);
    renderer.clear_lights();
    renderer.add_light(0, direction, ambient, diffuse, 0, 0);
    
    int party_light = 0, party_light_power = 0;
    if(focus_ && party_light_ && party_light_power_) {
        party_light = party_light_->execute(time_).as_int();
        party_light_power = party_light_power_->execute(time_).as_int();
    }
    
    if(party_light && party_light_power) {
        GLfloat position[] = { 0.0, 0.0, 0.0, 1.0 };
        GLfloat ambient[] = {0.0,0.0,0.0,0.0};
        GLfloat diffuse[] = {1.0,1.0,1.0,1.0};
        
        focus_->get_pos(position);
        position[2] += 1.0;
        diffuse[0] = GLfloat((party_light/10000)%100)/100.0;
        diffuse[1] = GLfloat((party_light/100)%100)/100.0;
        diffuse[2] = GLfloat(party_light%100)/100.0;
        
        
        renderer.add_light(4, position, ambient, diffuse, 1.0, 
                            GLfloat(party_light_power)/100.0);
    }
}

void world::fire_event(const std::string& name, const formula_callable& info)
{
	//std::cerr << "event: '" << name << "': " << variant(&info).to_debug_string() << "\n";
	std::pair<event_map::iterator,event_map::iterator> range =
	     handlers_.equal_range(name);
	while(range.first != range.second) {
            //std::cerr << "calling handler...\n";
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
    if(focus_ && focus_->is_human_controlled()) {
        return focus_;
    }
    
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

void world::get_inputs(std::vector<formula_input>* inputs) const
{
    inputs->push_back(formula_input("time", FORMULA_READ_ONLY));
    inputs->push_back(formula_input("ticks", FORMULA_READ_ONLY));
    inputs->push_back(formula_input("pc", FORMULA_READ_ONLY));
    inputs->push_back(formula_input("parties", FORMULA_READ_ONLY));
    inputs->push_back(formula_input("focus", FORMULA_READ_WRITE));
}

variant world::get_value(const std::string& key) const
{
    if(key == "time") {
        return variant(new game_time(time_));
    } else if(key == "ticks") {
        return variant(SDL_GetTicks());
    } else if(key == "pc") {
        return variant(get_pc_party().get());
    } else if(key == "parties") {
        std::vector<variant> parties;
        for(party_map::const_iterator i = parties_.begin(); i != parties_.end(); ++i) {
            parties.push_back(variant(i->second.get()));
        }
        
        return variant(&parties);
    } else if(key == "focus") {
        return variant(focus_.get());
    } else {
        return variant();
    }
}

void world::set_value(const std::string& key, const variant& value)
{
    if(key == "focus") {
        party* p = value.try_convert<party>();
        if(p) {
            focus_ = p;
        }
    }
    
    std::cerr << "unrecognized world value being set: '" << key << "'\n";
}

void world::add_chat_label(gui::label_ptr label, const_character_ptr ch, int delay)
{
    party_ptr pc = get_pc_party();
    if(!pc) {
        std::cerr << "failed to get pc party\n";
        return;
    }
    
    if(!game_bar_) {
        std::cerr << "failed to find game bar\n";
        return;
    }
    
    int most_recent = -1;
    foreach(const chat_label& lb, chat_labels_) {
        if(lb.character == ch && lb.started_at > most_recent) {
            most_recent = lb.started_at;
        }
    }
    
    if(most_recent != -1) {
        const int earliest = most_recent + 3000;
        const int ticks = SDL_GetTicks() + delay;
        if(earliest > ticks) {
            delay += earliest - ticks;
        }
    }
    
    std::vector<character_ptr>::const_iterator ch_itor = std::find(pc->begin_members(), pc->end_members(), ch);
    if(ch_itor == pc->end_members()) {
        return;
    }
    
    const int index = ch_itor - pc->begin_members();
    const SDL_Rect rect = game_bar_->character_rect(index);
    label->set_fixed_width(true);
    label->set_loc(rect.x, rect.y - 50);
    label->set_dim(rect.w, 100);
    
    chat_label new_label;
    new_label.label = label;
    new_label.started_at = SDL_GetTicks() + delay;
    new_label.y_loc = rect.y - 50;
    new_label.character = ch;
    chat_labels_.push_back(new_label);
}

world::listener::listener(world *wld) : listener_(this), wld_(wld) {
    listener_.set_target_state(SDL_BUTTON(SDL_BUTTON_LEFT), 
                               input::mouse_listener::STATE_MASK_NONE);
};

void world::listener::reset() {
    wld_->renderer_.reset_timing();
}

bool world::listener::process_event(const SDL_Event& e, bool claimed) {
    claimed |= listener_.process_event(e, claimed);
    return claimed;
}

void world::listener::click(Sint32 x, Sint32 y, int clicks, 
                            Uint8 state, Uint8 bstate, SDLMod mod) {
#if 0
    hex::location click_hex = wld_->selected_hex_;

    wld_->selected_hex_up_to_date_ = false;
    if(wld_->selection_.get_selected_hex() != click_hex) {
        return;
    }
#endif
    wld_->focus_->set_destination(wld_->selection_.get_selected_hex());        
}

}
