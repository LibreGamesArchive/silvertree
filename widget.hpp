
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef WIDGET_HPP_INCLUDED
#define WIDGET_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <string>

#include "SDL.h"

namespace gui {

class widget
{
public:
	void process_event(const SDL_Event& event);
	void draw() const;

	virtual void set_loc(int x, int y) { x_ = x; y_ = y; }
	virtual void set_dim(int w, int h) { w_ = w; h_ = h; }

	int x() const { return x_; }
	int y() const { return y_; }
	int width() const { return w_; }
	int height() const { return h_; }
	void set_tooltip(const std::string& str);
	bool visible() { return visible_; }
	void set_visible(bool visible) { visible_ = visible; }
protected:
	widget() : x_(-1), y_(-1), w_(-1), h_(-1), tooltip_displayed_(false), visible_(true)
	{}
	~widget();

	void normalize_event(SDL_Event* event);
private:
	virtual void handle_draw() const = 0;
	virtual void handle_event(const SDL_Event& event) {}
	int x_, y_;
	int w_, h_;
	boost::shared_ptr<std::string> tooltip_;
	bool tooltip_displayed_;
	bool visible_;
};

typedef boost::shared_ptr<widget> widget_ptr;
typedef boost::shared_ptr<const widget> const_widget_ptr;

}

#endif
