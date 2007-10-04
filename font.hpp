
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef FONT_HPP_INCLUDED
#define FONT_HPP_INCLUDED

#include <SDL.h>
#include <string>
#include <vector>

#include "surface.hpp"
#include "texture.hpp"

namespace graphics
{

namespace font
{

struct manager
{
	manager();
	~manager();
};

texture render_text(const std::string& text, int font_size,
                    const SDL_Color& color);
std::string format_text(const std::string& text, int font_size, int width);
void render_multiline_text(const std::string& text, int font_size,
                    const SDL_Color& color, std::vector<texture>& res,
					int *width=NULL, int *height=NULL);
void get_text_size(const std::string& text, int font_size, 
		   int *width = NULL, int *height =NULL);

texture render_complex_text(const std::string& text, int font_size, 
			    const SDL_Color& text_color, 
			    const SDL_Color& caret_fg, const SDL_Color& caret_bg, 
			    bool opaque_caret,
			    const SDL_Color& selection_fg, const SDL_Color& selection_bg, 
			    bool opaque_selection,
			    int caret, int selection_start, int selection_end);
}
		
}

#endif
