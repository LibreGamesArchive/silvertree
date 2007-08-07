
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

namespace gui {

class label;

typedef boost::shared_ptr<label> label_ptr;
typedef boost::shared_ptr<const label> const_label_ptr;

class label : public widget
{
public:
	static label_ptr create(const std::string& text,
	                        const SDL_Color& color, int size=14) {
		return label_ptr(new label(text, color, size));
	}
	label(const std::string& text, const SDL_Color& color, int size=14);

	void set_color(const SDL_Color& color);
	void set_text(const std::string& text);
	
private:
	void recalculate_texture();
	void handle_draw() const;

	std::string text_;
	graphics::texture texture_;
	SDL_Color color_;
	int size_;
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
