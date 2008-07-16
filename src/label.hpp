
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef LABEL_HPP_INCLUDED
#define LABEL_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include "SDL.h"

#include "texture.hpp"
#include "widget.hpp"
#include "text.hpp"

namespace gui {

class label;
class dialog_label;

typedef boost::shared_ptr<label> label_ptr;
typedef boost::shared_ptr<const label> const_label_ptr;
typedef boost::shared_ptr<dialog_label> dialog_label_ptr;

class label : public widget
{
public:
	static label_ptr create(const std::string& text,
	                        const SDL_Color& color, int size=14) {
		return label_ptr(new label(text, color, size));
	}
	label(const std::string& text, const SDL_Color& color, int size=14);

	void set_font_size(int font_size);
	void set_color(const SDL_Color& color);
	void set_text(const std::string& text);
	void set_fixed_width(bool fixed_width);
	virtual void set_dim(int x, int y);
	SDL_Color color() { return color_; }
	int size() { return size_; }
	std::string text() { return text_; }
protected:
	std::string& current_text();
	void set_rendered_text(text::rendered_text_ptr);
	virtual void recalculate_texture();
    void inner_set_dim(int w, int h);
private:
	void handle_draw() const;
	void reformat_text();

	std::string text_, formatted_;
	text::rendered_text_ptr rendered_text_;
	SDL_Color color_;
	int size_;
	bool fixed_width_;
};

class dialog_label : public label
{
public:
	dialog_label(const std::string& text, const SDL_Color& color, int size=18);
	void set_progress(int progress);
	int get_max_progress() { return stages_; }

protected:
	virtual void recalculate_texture();
private:

	int progress_, stages_;
};

class label_factory
{
public:
	label_factory(const SDL_Color& color, int size);
	label_ptr create(const std::string& text) const;
	label_ptr create(const std::string& text,
			 const std::string& tip) const;
private:
	SDL_Color color_;
	int size_;
};

}

#endif
