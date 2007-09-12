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

mini_stats_dialog::mini_stats_dialog(game_logic::battle_character_ptr ch, int w, int h)
	: framed_dialog(0,0,w,h), ch_(ch)
{ 
  construct_interface();
}

void mini_stats_dialog::prepare_draw() const {
	GLfloat box[4];
	ch_->loc_tracker().get_bbox(box);

	glPushMatrix();
	glTranslatef(floor(box[2]), floor(box[1]), 0);
}

void mini_stats_dialog::finish_draw() const {
	glPopMatrix();
}

void mini_stats_dialog::inner_draw() const {
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

	gui::label_ptr stats_label(new gui::label(ch_->status_text(), graphics::color_white()));
	add_widget(stats_label);
}
}
