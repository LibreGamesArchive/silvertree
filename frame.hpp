/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef FRAME_HPP_INCLUDED
#define FRAME_HPP_INCLUDED

#include <map>
#include <string>
#include <iostream>

#include "SDL.h"

#include "texture.hpp"
#include "widget.hpp"
#include "formula.hpp"
#include "font.hpp"
#include "wml_node.hpp"

namespace gui {

namespace frame_manager {

namespace {
typedef boost::shared_ptr<game_logic::formula> formula_ptr;
}

class frame;
typedef boost::shared_ptr<frame> frame_ptr;

class frame_draw_object {
public:
	virtual ~frame_draw_object() {}
	virtual void draw() const = 0;
};

class texture_fdo: public frame_draw_object {
public:
	texture_fdo(int x, int y, int w, int h,
		    const std::string& texture, bool tile) :
		texture_(), tile_(tile)
	{
		r_.x = x; r_.y = y;
		r_.w = w; r_.h = h;
		texture_ = graphics::texture::get(texture);
	}
	void draw() const;
private:
	graphics::texture texture_;
	SDL_Rect r_;
	bool tile_;
};

class rect_fdo: public frame_draw_object {
public:
	rect_fdo(int x, int y, int w, int h, int fill, SDL_Color& color, int alpha) :
		alpha_(alpha), fill_(fill), color_(color) {
		r_.x = x; r_.y = y;
		r_.w = w; r_.h = h;
	}
	void draw() const;
private:
	SDL_Rect r_;
	Uint8 alpha_;
	bool fill_;
	SDL_Color color_;
};

class text_fdo: public frame_draw_object {
public:
	text_fdo(int x, int y, int size, SDL_Color& color, const std::string& text) :
		x_(x), y_(y), texture_() {
		texture_ = graphics::font::render_text(text, size, color);
	}
	void draw() const;
private:
	int x_, y_;
	graphics::texture texture_;
};

typedef boost::shared_ptr<frame_draw_object> fdo_ptr;


class rect_mapper: public game_logic::formula_callable {
public:
	rect_mapper(const SDL_Rect& rect);
private:
	variant get_value(const std::string& key) const;

	int x_, y_, w_, h_;
};

class key_mapper: public game_logic::formula_callable {
public:
	void add_rect(const std::string& name, int x, int y, int w, int h);
	void remove_rect(const std::string& name);
private:
	variant get_value(const std::string& key) const;

	std::map<std::string,boost::shared_ptr<rect_mapper> > rects_;
};


typedef boost::shared_ptr<key_mapper> key_mapper_ptr;

/* exception class */
class invalid_type {};

class frame_draw_builder {
public:
	typedef enum { ATTR_UNKNOWN=0, ATTR_STRING, ATTR_BOOL, ATTR_INT } attr_type;

	virtual ~frame_draw_builder() {}
	void init_from_wml(const wml::const_node_ptr& node);
	virtual fdo_ptr make_fdo(const key_mapper_ptr& keys) const =0;
	void set_attr(const std::string& name, const std::string& value);
	int get_int_attr(const std::string& name, const key_mapper_ptr& keys, int deflt) const;
	int get_int_attr(const std::string& name, const key_mapper_ptr& keys, int deflt, int min, int max) const;
	std::string get_string_attr(const std::string& name, const key_mapper_ptr& keys, 
				    const std::string& deflt) const;
	bool get_bool_attr(const std::string& name, const key_mapper_ptr& keys, bool deflt) const;
protected:
	virtual attr_type get_attr_type(const std::string& attribute) const=0;
private:
	formula_ptr get_formula_attr(const std::string& name) const;
	std::map<std::string,formula_ptr> formula_attrs_;
	std::map<std::string,std::string> string_attrs_;
};

class texture_fdb: public frame_draw_builder {
public:
	fdo_ptr make_fdo(const key_mapper_ptr& keys) const;
protected:
	attr_type get_attr_type(const std::string& name) const;
};

class rect_fdb: public frame_draw_builder {
public:
	fdo_ptr make_fdo(const key_mapper_ptr& keys) const;
protected:
	attr_type get_attr_type(const std::string& name) const;
};

class text_fdb: public frame_draw_builder {
public:
	fdo_ptr make_fdo(const key_mapper_ptr& keys) const;
protected:
	attr_type get_attr_type(const std::string& name) const;
};

typedef boost::shared_ptr<frame_draw_builder> fdb_ptr;

class frame_builder {
public:
	void initialise_frame(frame *fr) const;
	void set_attr(const std::string& name, formula_ptr value) { formula_map_[name] = value; }
	void add_predraw(fdb_ptr pre) { predraw_.push_back(pre); }
	void add_postdraw(fdb_ptr post) { postdraw_.push_back(post); }
private:
	std::map<std::string, formula_ptr> formula_map_;
	std::vector<fdb_ptr> predraw_;
	std::vector<fdb_ptr> postdraw_;
};

typedef boost::shared_ptr<frame_builder> fb_ptr;

frame_ptr make_frame(widget_ptr w, const std::string& name, key_mapper_ptr keys);
frame_ptr make_frame(widget_ptr w, const std::string& name);
void flush_frame(const std::string& name);
void flush_frames();

class frame: public widget {
public:
	frame(widget_ptr base, fb_ptr builder, key_mapper_ptr keys);
	virtual void set_loc(int x, int y);
	virtual void set_dim(int w, int h);
	void set_bg(const SDL_Color& bg) { bg_ = bg; }
	void set_fg(const SDL_Color& fg) { fg_ = fg; }
	void set_fixed_width(bool fixed_width) { fixed_width_ = fixed_width; }
	void set_border_left(int left) { border_l_ = left; }
	void set_border_right(int right) { border_r_ = right; }
	void set_border_top(int top) { border_t_ = top; }
	void set_border_bottom(int bottom) { border_b_ = bottom; }
	int border_left() { return border_l_; }
	int border_right() { return border_r_; }
	int border_width() { return border_l_ + border_r_; }
	int border_top() { return border_t_; }
	int border_bottom() { return border_b_; }
	int border_height() { return border_t_ + border_b_; }
	widget_ptr base() { return base_; }
	void add_predraw(fdo_ptr fdo);
	void add_postdraw(fdo_ptr fdo);
	const key_mapper_ptr get_keys() { return keys_; }
	void add_key_set(const std::string& name, int x, int y, int w, int h);
private:
	virtual void handle_draw() const;
	virtual void handle_event(const SDL_Event& event);
	void calculate_loc();
	void calculate_dim();
	void rebuild_frame();

	std::vector<fdo_ptr> predraw_, postdraw_;
	widget_ptr base_;
	int border_t_, border_b_, border_l_, border_r_;
	SDL_Color fg_, bg_;
	bool fixed_width_, basic_;
	fb_ptr builder_;
	key_mapper_ptr keys_;
};

}

typedef boost::shared_ptr<frame_manager::frame> frame_ptr;

}

#endif
