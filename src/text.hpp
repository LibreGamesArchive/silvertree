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

#include <memory>
#include <string>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "gl_utils.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftbitmap.h>

namespace graphics
{

namespace text
{

struct ft_bitmap_handle
{
	FT_Bitmap bitmap;

	ft_bitmap_handle(int width, int height);
	~ft_bitmap_handle();

	private:
	ft_bitmap_handle(const ft_bitmap_handle&);
};

typedef boost::shared_ptr<ft_bitmap_handle> ft_bitmap_ptr;

class renderer
{
	static boost::scoped_ptr<renderer> the_renderer;

	protected:
	renderer();

	public:
	~renderer();

	static renderer& instance();

	gl::texture2d_ptr render_to_texture(std::string text, int size = 14, bool markup = false);
	ft_bitmap_ptr render(std::string text, int size = 14, bool markup = false);
};

}

}

#endif
