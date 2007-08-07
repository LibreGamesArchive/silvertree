
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "SDL_ttf.h"

#include <boost/shared_ptr.hpp>
#include <iostream>
#include <map>
#include <string.h>

#include "font.hpp"
#include "foreach.hpp"
#include "sdl_algo.hpp"
#include "string_utils.hpp"

namespace graphics
{

namespace font
{

namespace {

typedef boost::shared_ptr<TTF_Font> font_ptr;

typedef std::map<int,font_ptr> font_map;
font_map fonts;
bool fonts_initialized = false;

struct release_font
{
	void operator()(TTF_Font* font) const
	{
		if(font != NULL) {
			TTF_CloseFont(font);
		}
	}
};

font_ptr get_font(int size)
{
	if(!fonts_initialized) {
		return font_ptr();
	}
	
	const font_map::iterator i = fonts.find(size);
	if(i != fonts.end()) {
		return i->second;
	} else {
		font_ptr new_font(TTF_OpenFont("FreeSans.ttf",size),
		                  release_font());
		if(!new_font) {
			std::cerr << "ttf open failed: " << TTF_GetError() << "\n";
		}

		fonts.insert(std::pair<int,font_ptr>(size,new_font));
		return new_font;
	}
}
		
}

manager::manager()
{
	TTF_Init();
	fonts_initialized = true;
}

manager::~manager()
{
	fonts.clear();
	TTF_Quit();
	fonts_initialized = false;
}

texture render_text(const std::string& text, int font_size,
                    const SDL_Color& color)
{
	const font_ptr font(get_font(font_size));
	if(!font) {
		return texture();
	}

	if(strchr(text.c_str(),'\n') != NULL) {
		std::vector<std::string> v = util::split(text, '\n');
		std::vector<surface> surfs;
		unsigned int width = 0, height = 0;
		foreach(const std::string& s, v) {
			surface surf(get_non_transparent_portion(
			  TTF_RenderText_Blended(font.get(),s.c_str(),color)));
			surfs.push_back(surf);
			if(surf->w > width) {
				width = surf->w;
			}

			height += surf->h;
		}

		surface res(SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,
		                     32,SURFACE_MASK));
		int y = 0;
		foreach(const surface& surf, surfs) {
			SDL_SetAlpha(surf.get(), 0, SDL_ALPHA_OPAQUE);
			SDL_Rect rect = {0,y,surf->w,surf->h};
			SDL_BlitSurface(surf.get(), NULL, res.get(), &rect);
			y += surf->h;
		}

		return texture::get_no_cache(res);
	}

	const surface res(get_non_transparent_portion(TTF_RenderText_Blended(
	        font.get(),text.c_str(),color)));
	if(res.get() != NULL) {
		return texture::get_no_cache(res);
	} else {
		return texture();
	}
}

void render_multiline_text(const std::string& text, int font_size,
                    const SDL_Color& color, std::vector<texture>& res,
					int* width, int* height)
{
	if(width) {
		*width = 0;
	}

	if(height) {
		*height = 0;
	}
	std::vector<std::string> v = util::split(text, '\n');
	foreach(const std::string& s, v) {
		res.push_back(render_text(s,font_size,color));
		if(width && res.back().width() > *width) {
			*width = res.back().width();
		}

		if(height) {
			*height += res.back().height();
		}
	}
}
		
} //end namespace font

} //end namespace graphics
