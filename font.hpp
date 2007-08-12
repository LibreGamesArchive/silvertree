
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
void render_multiline_text(const std::string& text, int font_size,
                    const SDL_Color& color, std::vector<texture>& res,
					int *width=NULL, int *height=NULL);

}
		
}

#endif
