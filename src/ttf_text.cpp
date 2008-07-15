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

#include "filesystem.hpp"
#include "foreach.hpp"
#include "sdl_algo.hpp"
#include "string_utils.hpp"
#include "ttf_text.hpp"

//#ifdef __APPLE__
#define SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
//#endif

namespace text
{

namespace ttf {

typedef boost::shared_ptr<TTF_Font> font_ptr;

typedef std::map<int,font_ptr> font_map;
font_map* fonts = NULL;
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
    
    const font_map::iterator i = fonts->find(size);
    if(i != fonts->end()) {
        return i->second;
    } else {
        font_ptr new_font(TTF_OpenFont(sys::find_file("FreeSans.ttf").c_str(),size),
                          release_font());
        if(!new_font) {
            std::cerr << "ttf open failed: " << TTF_GetError() << "\n";
        }
        
        fonts->insert(std::pair<int,font_ptr>(size,new_font));
        return new_font;
    }
}

renderer::renderer()
{
	fonts = new font_map;
    TTF_Init();
    fonts_initialized = true;
}

renderer::~renderer()
{
	delete fonts;
    TTF_Quit();
    fonts_initialized = false;
}

int renderer::get_string_height(const std::string& text, int font_size) {
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

rendered_text_ptr renderer::render(const std::string& text, int font_size,
                                           const SDL_Color& color)
{
    const font_ptr font(get_font(font_size));
    if(!font) {
        return rendered_text_ptr(new rendered_text(graphics::texture()));
    }
    
    std::vector<std::string> v = util::split(text, '\n');
    std::vector<graphics::surface> surfs;
    unsigned int width = 0, height = 0;
    
#ifndef SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
    const unsigned int lineskip = TTF_FontLineSkip(font.get());
    std::vector<int> heights;
    int ascent = TTF_FontAscent(font.get());
#endif
    foreach(const std::string& s, v) {
        graphics::surface surf(graphics::get_non_transparent_portion(
                                TTF_RenderUTF8_Blended(font.get(),s.c_str(),color)));
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

	graphics::surface res(SDL_CreateRGBSurface(SDL_SWSURFACE,width,height,
                                                   32,SURFACE_MASK));
	int y = 0;
#ifndef SDL_TTF_RENDERS_TO_CONSTANT_SIZE_BOX
	std::vector<int>::iterator iter = heights.begin();
#endif
	foreach(const graphics::surface& surf, surfs) {
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
	return rendered_text_ptr(new rendered_text(graphics::texture::get_no_cache(res, 1 << graphics::texture::NO_MIPMAP)));
}

std::string renderer::format(const std::string& text, int font_size, int width)
{
    const font_ptr font(get_font(font_size));
    if(!font) {
        return text;
    }
    
    int space_width, junk;
    TTF_SizeUTF8(font.get(), " ", &space_width, &junk);
    
    std::string formatted;
    
    std::vector<std::string> lines = util::split(text, '\n');
    std::vector<std::string>::iterator itor;
    for(itor = lines.begin(); itor != lines.end(); ++itor) {
        int line_words = 0;
        int line_width = 0;
        std::string& text_line = *itor;
        std::vector<std::string> words = util::split(text_line, "\n\t ");
        foreach(const std::string& word, words) {
            int w;
            if(TTF_SizeUTF8(font.get(), word.c_str(), &w, &junk) < 0) {
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
        if(!formatted.empty() && itor+1 != lines.end()) {
            formatted.append(1,'\n');
        }
        
    }
    return formatted;

}

struct chunk_t {
	int start, end; /* inclusive */
	bool has_bg;
	SDL_Color fg;
	SDL_Color bg;
	int x,y; /* chunk render pos; y unused for now */
};

inline void chunk(int start, int end, const SDL_Color& fg, const SDL_Color& bg, bool has_bg, std::vector<chunk_t>& chunks) {
	chunk_t tmp;
	if(end < start) {
		return;
	}
	if(start < 0) start = 0;
	/* end not clipped (no context) */
	tmp.start=  start;
	tmp.end = end;
	tmp.fg = fg;
	tmp.bg = bg;
	tmp.has_bg = has_bg;
	tmp.x = 0;
	tmp.y = 0;
	chunks.push_back(tmp);
}

/* This does not handle newlines */
rendered_text_ptr renderer::render_complex_text(const std::string& text, int font_size,
                                                const SDL_Color& text_color,
                                                const SDL_Color& caret_fg, const SDL_Color& caret_bg,
                                                bool opaque_caret,
                                                const SDL_Color& selection_fg, const SDL_Color& selection_bg,
                                                bool opaque_selection,
                                                int caret, int selection_start, int selection_end)
{
    std::vector<chunk_t> chunks;
    const std::string used_text = text + " ";
    const int max_idx = used_text.size()-1;
    const font_ptr font(get_font(font_size));
    
    if(!font) {
        return rendered_text_ptr(new rendered_text(graphics::texture()));
    }
    
    if(caret < 0) {
        if(selection_start < 0) {
            /* no selection, no caret */
            chunk(0, max_idx, text_color, text_color, false, chunks);
		} else {
                    /* no caret, selection */
                    const int end = selection_end < 0 ? used_text.size() -1 : selection_end;
                    chunk(0, selection_start -1, text_color, text_color, false, chunks);
                    chunk(selection_start, end, selection_fg, selection_bg, opaque_selection, chunks);
                    chunk(end+1, max_idx, text_color, text_color, false, chunks);
		}
    } else {
        if(selection_start < 0) {
            /* caret, no selection */
            chunk(0, caret-1, text_color, text_color, false, chunks);
            chunk(caret, caret, caret_fg, caret_bg, opaque_caret, chunks);
            chunk(caret+1, max_idx, text_color, text_color, false, chunks);
        } else {
            /* most complex case selection & caret */
            const int end = selection_end < 0 ? used_text.size() -1 : selection_end;
            if(caret < selection_start) {
                chunk(0, caret-1, text_color, text_color, false, chunks);
                chunk(caret, caret, caret_fg, caret_bg, opaque_caret, chunks);
                chunk(caret+1, selection_start-1, text_color, text_color, false, chunks);
				chunk(selection_start, end, selection_fg, selection_bg, opaque_selection, chunks);
				chunk(end+1, max_idx, text_color, text_color, false, chunks);
            } else if(caret > end) {
                chunk(0, selection_start-1, text_color, text_color, false, chunks);
                chunk(selection_start, end, selection_fg, selection_bg, opaque_selection, chunks);
                chunk(end+1, caret-1, text_color, text_color, false, chunks);
                chunk(caret, caret, caret_fg, caret_bg, opaque_caret, chunks);
                chunk(caret+1, max_idx, text_color, text_color, false, chunks);
            } else {
                chunk(0, selection_start-1, text_color, text_color, false, chunks);
                chunk(selection_start, caret-1, selection_fg, selection_bg, opaque_selection, chunks);
                chunk(caret, caret, caret_fg, caret_bg, opaque_caret, chunks);
                chunk(caret+1, end, selection_fg, selection_bg, opaque_selection, chunks);
                chunk(end+1, max_idx, text_color, text_color, false, chunks);
            }
		}
    }
    /* now we have a correctly constructed chunks array, but missing positions */
    std::string text_so_far;
    int w = 0;
    
    foreach(chunk_t& c, chunks) {
        c.end = c.end < max_idx ? c.end : max_idx;
        const int len = c.end - c.start + 1; /* points are inclusive */
        std::string chunk_text = used_text.substr(c.start, len);
        c.x = w;
        c.y = 0;
        /* do it like this to get correct kerning (the _only_ way in
           any released version of SDL_ttf) */
        text_so_far.append(chunk_text);
        int junk;
        TTF_SizeUTF8(font.get(), text_so_far.c_str(), &w, &junk);
    }
    
    /* now we have a constructed chunks array and need to render it */
    graphics::surface res(SDL_CreateRGBSurface(SDL_SWSURFACE,w,TTF_FontHeight(font.get()),32,SURFACE_MASK));
    foreach(chunk_t& c, chunks) {
        std::string chunk_text = used_text.substr(c.start, c.end - c.start +1);
        graphics::surface surf;
        if(c.has_bg) {
            surf = TTF_RenderUTF8_Shaded(font.get(), chunk_text.c_str(), c.fg, c.bg);
        } else {
            surf = TTF_RenderUTF8_Blended(font.get(),chunk_text.c_str(), c.fg);
        }
        if(surf) {
            SDL_SetAlpha(surf.get(), 0, SDL_ALPHA_OPAQUE);
            SDL_Rect r = { c.x, c.y, surf->w, surf->h };
            SDL_BlitSurface(surf.get(), NULL, res.get(), &r);
        }
    }

    return rendered_text_ptr(new rendered_text(graphics::texture::get_no_cache(res, 1 << graphics::texture::NO_MIPMAP)));
}

void renderer::get_text_size(const std::string& text, int font_size, int *w, int *h)
{
    const font_ptr font(get_font(font_size));
    TTF_SizeUTF8(font.get(), text.c_str(), w ,h);
}

} //end namespace ttf

} //end namespace text
