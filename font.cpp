
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

//#ifdef __APPLE__
#define SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
//#endif

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

int get_string_height(const std::string& text, int font_size) {
	const font_ptr font(get_font(font_size));
	if(!font) {
		return -1;
	}

	std::string::const_iterator iter = text.begin();
	int ret = 0;
	while(iter != text.end()) {
		int max_y;
		TTF_GlyphMetrics(font.get(), *iter, NULL, NULL, NULL, &max_y, NULL);
		if(max_y > ret) {
			ret = max_y;
		}
		++iter;
	}
	return ret;
}

texture render_text(const std::string& text, int font_size,
                    const SDL_Color& color)
{
	const font_ptr font(get_font(font_size));
	if(!font) {
		return texture();
	}

	std::vector<std::string> v = util::split(text, '\n');
	std::vector<surface> surfs;
	unsigned int width = 0, height = 0;

#ifndef SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
	const unsigned int lineskip = TTF_FontLineSkip(font.get());
	std::vector<int> heights;
	int ascent = TTF_FontAscent(font.get());
#endif
	foreach(const std::string& s, v) {
		surface surf(get_non_transparent_portion(
				     TTF_RenderText_Blended(font.get(),s.c_str(),color)));
		surfs.push_back(surf);
		if(surf->w > width) {
			width = surf->w;
		}
#ifndef SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
		heights.push_back(get_string_height(s, font_size));
		height += lineskip;
#else
		height += surf->h;
#endif
	}

	
	surface res(SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,
					 32,SURFACE_MASK));
	int y = 0;
#ifndef SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
	std::vector<int>::iterator iter = heights.begin();
#endif
	foreach(const surface& surf, surfs) {
		SDL_SetAlpha(surf.get(), 0, SDL_ALPHA_OPAQUE);

#ifndef SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
		int y_adjust = ascent - *(iter++);
		SDL_Rect rect = {0,y + y_adjust,surf->w,surf->h};
#else
		SDL_Rect rect = {0,y,surf->w,surf->h};
#endif
		SDL_BlitSurface(surf.get(), NULL, res.get(), &rect);
#ifndef SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
		y += lineskip;
#else
		y += surf->h;
#endif
	}
	return texture::get_no_cache(res, 1 << texture::NO_MIPMAP);
}

std::string format_text(const std::string& text, int font_size, int width)
{
	const font_ptr font(get_font(font_size));
	if(!font) {
		return text;
	}

	int space_width, junk;
	TTF_SizeText(font.get(), " ", &space_width, &junk);

	std::string formatted;

	std::vector<std::string> lines = util::split(text, '\n');
	foreach(const std::string& text_line, lines) {
		int line_words = 0;
		int line_width = 0;
		std::vector<std::string> words = util::split(text_line, "\n\t ");
		foreach(const std::string& word, words) {
			int w;
			if(TTF_SizeText(font.get(), word.c_str(), &w, &junk) < 0) {
				std::cerr << "Warning: error rendering text \"" <<
					text_line << "\"\n";
				return text;
			}
			if(line_words > 0) {
				if(line_width + w > width) {
					formatted.append(1, '\n');
					line_width = 0;
					line_words = 0;
				} else {
					formatted.append(1, ' ');
				}
			}
			formatted.append(word);
			line_width += w + space_width;
			++line_words;
		}
		if(!formatted.empty()) {
		  formatted.append(1,'\n');
		}
	}
	return formatted;
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
