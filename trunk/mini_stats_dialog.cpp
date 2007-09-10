#include <math.h>
#include <sstream>

#include "character.hpp"
#include "raster.hpp"
#include "image_widget.hpp"
#include "mini_stats_dialog.hpp"

namespace gui {

void framed_dialog::handle_draw() const
{
	if(!frame_) {
		prepare_draw();
		inner_draw();
		finish_draw();
		return;
	}
	if(nested_draw_) {
		inner_draw();
	} else {
		prepare_draw();
		nested_draw_ = true;
		try {
			frame_->draw();
		} catch(...) {
			nested_draw_ = false;
			throw;
		}
		nested_draw_ = false;
		finish_draw();
	}
}

void framed_dialog::set_dim(int w, int h) 
{
	if(!frame_) {
		inner_set_dim(w,h);
		return;
	} 
	if(nested_set_dim_) {
		inner_set_dim(w,h);
	} else {
		nested_set_dim_ = true;
		try {
			frame_->set_dim(w,h);
		} catch(...) {
			nested_set_dim_ = false;
			throw;
		}
		nested_set_dim_ = false;
	}
}

void framed_dialog::inner_set_dim(int w, int h) {
	dialog::set_dim(w,h);
}

void framed_dialog::set_loc(int x, int y) 
{
	if(!frame_) {
		inner_set_loc(x,y);
		return;
	} 
	if(nested_set_loc_) {
		inner_set_loc(x,y);
	} else {
		nested_set_loc_ = true;
		try {
			frame_->set_loc(x,y);
		} catch(...) {
			nested_set_loc_ = false;
			throw;
		}
		nested_set_loc_ = false;
	}
}

void framed_dialog::inner_set_loc(int x, int y) {
	dialog::set_loc(x,y);
}

}

namespace game_dialogs {

mini_stats_dialog::mini_stats_dialog(game_logic::battle_character_ptr ch, 
				     int ttl, int w, int h)
	: framed_dialog(0,0,w,h), ch_(ch), ttl_(ttl) 
{ 
	if(ttl >= 0) {
		death_time_ = SDL_GetTicks() + ttl;
	} else {
		death_time_ = -1;
	}
	dead_ = false;
	construct_interface();
}

void mini_stats_dialog::refresh() 
{
	if(dead_) {
		return;
	}
	if(death_time_ >= 0) {
		death_time_ = SDL_GetTicks() + ttl_;
	}
}

bool mini_stats_dialog::hits_me(const SDL_Event& e) 
{
	GLfloat box[4];
	ch_->loc_tracker().get_bbox(box);

	int x1 = static_cast<int>(box[0]);
	int y1 = static_cast<int>(box[1]);
	int x2 = static_cast<int>(box[2]);
	int y2 = static_cast<int>(box[3]);

	int ex, ey;
	switch(e.type) {
	case SDL_MOUSEMOTION:
		ex = e.motion.x;
		ey = e.motion.y;
		break;
	case SDL_MOUSEBUTTONDOWN:
		ex = e.button.x;
		ey = e.button.y;
		break;
	default:
		return false;
	}
	if(ex > x1 && ey > y1 && ex < x2 && ey < y2) {
		return true;
	}
	return false;
}

void mini_stats_dialog::check_death() const {
	if(death_time_ >= 0 && death_time_ <= SDL_GetTicks()) {
		dead_ = true;
	}
}

void mini_stats_dialog::hide_and_close() {
	dead_ = true;
	set_visible(false);
	close();
}

void mini_stats_dialog::handle_event(const SDL_Event& e) { 
	check_death();

	if(dead_) {
		hide_and_close();
		return;
	}

	switch(e.type) {
	case SDL_MOUSEMOTION:
		if(!hits_me(e)) {
			hide_and_close();
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if(hits_me(e)) {
			hide_and_close();
		}
		break;
	case SDL_KEYDOWN:
		hide_and_close();
		break;
	default:
		break;
	}
}

void mini_stats_dialog::prepare_draw() const {
	GLfloat box[4];
	ch_->loc_tracker().get_bbox(box);

	glPushMatrix();
	glTranslatef(floor(box[0] - width()), floor(box[1]), 0);
}

void mini_stats_dialog::finish_draw() const {
	glPopMatrix();
}

void mini_stats_dialog::inner_draw() const {
	check_death();
	if(dead_) {
		return;
	}

	handle_draw_children();
}

void mini_stats_dialog::construct_interface() 
{
	game_logic::character& rch = ch_->get_character();
	
	std::string portrait_file = rch.portrait();
	if(!portrait_file.empty()) {
		gui::widget_ptr portrait = 
			gui::widget_ptr(new gui::image_widget(portrait_file, width()/2, height()));
		add_widget(portrait, dialog::MOVE_RIGHT);
	} else {
		set_dim(width()/2, height());
	}

	/*
	{
		gui::label_ptr name_label(new gui::label(rch.description(), graphics::color_yellow()));
		name_label->set_fixed_width(true);
		name_label->set_dim(width()*7/8, 0);
		add_widget(name_label, dialog::MOVE_DOWN);
	}
	*/
	{
		//set_cursor(0, height()/4);
		
		gui::label_ptr stats_label(new gui::label(ch_->status_text(), graphics::color_white()));
		add_widget(stats_label);
	}
}
}
