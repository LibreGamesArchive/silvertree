/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef MESSAGE_DIALOG_HPP_INCLUDED
#define MESSAGE_DIALOG_HPP_INCLUDED

#include <string>
#include <vector>

#include "party.hpp"
#include "dialog.hpp"
#include "world.hpp"
#include "widget.hpp"
#include "frame.hpp"
#include "label.hpp"

namespace gui {

class phantom_dialog: public dialog {
public:
	phantom_dialog() : dialog(0, 0, 0, 0) { set_clear_bg(false); }
	virtual dialog& add_widget(widget_ptr w, MOVE_DIRECTION dir=MOVE_DOWN);
	virtual dialog& add_widget(widget_ptr w, int x, int y,
	                MOVE_DIRECTION dir=MOVE_DOWN);
	virtual void remove_widget(widget_ptr w);
	virtual void replace_widget(widget_ptr w_old, widget_ptr w_new);
	virtual void clear();
private:
	void adapt_size();
};

class message_dialog: public dialog {
public:
	message_dialog(game_logic::party& pc,
		       game_logic::party& npc,
		       const std::string& msg,
		       const std::vector<std::string>* options=NULL,
		       bool starts_conversation = true);
	bool has_options() const { return NULL == options_; }
	int selected() const { return selected_; }
	void set_fade_in_rate(int rate) { fade_in_rate_ = rate; } 
	void show_modal();
protected:
	void handle_draw() const;
	void handle_event(const SDL_Event &event);
	void construct_interface();
	int find_option(int x, int y);
	frame_ptr make_option_frame(int opt, widget_ptr base, 
				    frame_manager::key_mapper_ptr keys);
	frame_ptr make_option_frame(int opt, widget_ptr base);
private:
	void fade_in();
	void ensure_fade_over();
	void update_option(int option);
	int selected_;
	const std::string& msg_;
	const std::vector<std::string>* options_;
	game_logic::party& pc_;
	game_logic::party& npc_;
	std::vector<frame_ptr> option_frames_;
	std::vector<boost::shared_ptr<dialog_label> > dialog_labels_;
	widget_ptr pc_portrait_;
	widget_ptr all_option_frame_;
	boost::shared_ptr<phantom_dialog> option_box_;
	int fade_in_rate_;
	bool starts_conversation_;
};

}

#endif
