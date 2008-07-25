/***************************************************************************
 *  Copyright (C) 2008 by Sergey Popov <loonycyborg@gmail.com>             *
 *                                                                         *
 *  This file is part of Silver Tree.                                      *
 *                                                                         *
 *  Silver Tree is free software; you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  Silver Tree is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/
#ifndef PANGO_TEXT_HPP_INCLUDED
#define PANGO_TEXT_HPP_INCLUDED

#include <memory>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftbitmap.h>

#include "text.hpp"

namespace text
{

namespace pango
{

struct ft_bitmap_handle
{
    FT_Bitmap bitmap_;
    ft_bitmap_handle(int width, int height);
    ~ft_bitmap_handle();
private:
    ft_bitmap_handle(const ft_bitmap_handle&);
};

typedef boost::shared_ptr<ft_bitmap_handle> ft_bitmap_ptr;

class rendered_text: public text::rendered_text {
public:
    rendered_text(boost::shared_array<unsigned char> pixels, int width, int height, bool colored, const SDL_Color& color) :
        pixels_(pixels), width_(width), height_(height), colored_(colored), color_(color) {}
    graphics::texture as_texture();
    void blit(int x=0, int y=0);
    int width() { return width_; }
    int height() { return height_; }
private:
    rendered_text();

    boost::shared_array<unsigned char> pixels_;
    int width_, height_;
    bool colored_;
    SDL_Color color_;
};

class renderer: public text::renderer
{
public:
    renderer();
    ~renderer();
    text::rendered_text_ptr render(const std::string& text, 
                             int size, 
                             const SDL_Color& color, bool markup = false);
    text::rendered_text_ptr render_complex_text(const std::string& text, int font_size,
                                                const SDL_Color& text_color,
                                                const SDL_Color& caret_fg, const SDL_Color& caret_bg,
                                                bool opaque_caret,
                                                const SDL_Color& selection_fg, const SDL_Color& selection_bg,
                                                bool opaque_selection,
                                                int caret, int selection_start, int selection_end);
    std::string format(const std::string& text, 
                       int font_size, 
                       int width);
    void get_text_size(const std::string& text, 
                       int font_size,
                       int *width = NULL, int *height =NULL);
    
private:
};

}

}

#endif
