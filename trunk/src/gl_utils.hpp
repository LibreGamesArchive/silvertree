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
#ifndef GL_UTILS_HPP_INCLUDED
#define GL_UTILS_HPP_INCLUDED

#include <GL/glew.h>
#include <boost/shared_ptr.hpp>

namespace graphics
{

namespace gl
{

template<GLenum dim> class texture
{
	GLuint id;
	public:
	texture() { glGenTextures(1, &id); }
	~texture() { glDeleteTextures(1, &id); }

	void bind() { glBindTexture(dim, id); }

	protected:
	texture(const texture&);
};

typedef texture<GL_TEXTURE_2D> texture2d;
typedef boost::shared_ptr<texture2d> texture2d_ptr;

}

}

#endif
