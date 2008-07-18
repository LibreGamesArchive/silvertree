
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TTF_TEXT_HPP_INCLUDED
#define TTF_TEXT_HPP_INCLUDED

#include <SDL.h>
#include <string>
#include <vector>

#include "raster.hpp"
#include "surface.hpp"
#include "text.hpp"
#include "texture.hpp"

namespace text
{

namespace ttf 
{

class rendered_text: public text::rendered_text {
public:
    rendered_text(graphics::texture texture) :
        texture_(texture) {}
    void blit(int x=0, int y=0) {
        graphics::blit_texture(texture_, x, y);
    }
    int width() { return texture_.width(); }
    int height() { return texture_.height(); }
    graphics::texture as_texture() {
        return texture_;
    }
private:
    graphics::texture texture_;
};

class renderer: public text::renderer {
public:
    renderer();
    ~renderer();

    text::rendered_text_ptr render(const std::string& text, 
                                   int font_size,
                                   const SDL_Color& color, bool markup = false);
    std::string format(const std::string& text, 
                       int font_size,
                       int width);
    void get_text_size(const std::string& text, int font_size,
                       int *width = NULL, int *height =NULL);
    text::rendered_text_ptr render_complex_text(const std::string& text, 
                                                int font_size,
                                                const SDL_Color& text_color,
                                                const SDL_Color& caret_fg, const 
                                                SDL_Color& caret_bg,
                                                bool opaque_caret,
                                                const SDL_Color& selection_fg, const SDL_Color& selection_bg,
                                                bool opaque_selection,
                                                int caret, int selection_start, int selection_end);
protected:
    int get_string_height(const std::string& text, int font_size);
};

}

}

#endif
