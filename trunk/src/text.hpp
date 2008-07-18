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
#ifndef TEXT_HPP_INCLUDED
#define TEXT_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "SDL.h"
#include "texture.hpp"

namespace text {

class rendered_text {
public:
    virtual ~rendered_text() {}
    virtual void blit(int x=0, int y=0) =0;
    virtual int width()=0;
    virtual int height()=0;
    virtual graphics::texture as_texture()=0;
};

typedef boost::shared_ptr<rendered_text> rendered_text_ptr;

class renderer
{
public:
    virtual ~renderer() {}

    static renderer& instance();
    virtual rendered_text_ptr render(const std::string& text, 
                                     int size, 
                                     const SDL_Color& color, bool markup = false) =0;
    virtual std::string format(const std::string& text, 
                               int font_size, 
                               int width)=0;
    virtual void get_text_size(const std::string& text, 
                               int font_size,
                               int *width = NULL, int *height =NULL) =0;
    virtual rendered_text_ptr render_complex_text(const std::string& text, 
                                                  int font_size,
                                                  const SDL_Color& text_color,
                                                  const SDL_Color& caret_fg, const 
                                                  SDL_Color& caret_bg,
                                                  bool opaque_caret,
                                                  const SDL_Color& selection_fg, const SDL_Color& selection_bg,
                                                  bool opaque_selection,
                                                  int caret, int selection_start, int selection_end)=0;
protected:
    renderer() {};
private:
	static boost::scoped_ptr<renderer> renderer_;

	renderer(const renderer&);
	renderer& operator=(const renderer&);
};

typedef boost::shared_ptr<renderer> renderer_ptr;

}

#endif
