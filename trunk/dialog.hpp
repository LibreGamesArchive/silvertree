
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef DIALOG_HPP_INCLUDED
#define DIALOG_HPP_INCLUDED

#include "callback.hpp"
#include "widget.hpp"

#include <string>
#include <vector>

namespace gui {

int show_dialog(const std::string& msg,
                const std::vector<std::string>* options=NULL);

class dialog : public widget
{
public:
	virtual ~dialog() {}
	void show_modal();

	enum MOVE_DIRECTION { MOVE_DOWN, MOVE_RIGHT };
	dialog& add_widget(widget_ptr w, MOVE_DIRECTION dir=MOVE_DOWN);
	dialog& add_widget(widget_ptr w, int x, int y,
	                MOVE_DIRECTION dir=MOVE_DOWN);
	void remove_widget(widget_ptr w);
	void clear() { widgets_.clear(); }
	void set_padding(int pad) { padding_ = pad; }
	void close() { opened_ = false; }

	void set_cursor(int x, int y) { add_x_ = x; add_y_ = y; }
	int cursor_x() const { return add_x_; }
	int cursor_y() const { return add_y_; }
protected:
	dialog(int x, int y, int w, int h);
	virtual void handle_event(const SDL_Event& event);
	virtual void handle_draw() const;
private:
	std::vector<widget_ptr> widgets_;
	bool opened_;

	//default padding between widgets
	int padding_;

	//where the next widget will be placed by default
	int add_x_, add_y_;
};

DEFINE_CALLBACK(close_dialog_callback, dialog, close);

}

#endif