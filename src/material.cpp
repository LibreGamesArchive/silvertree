
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "material.hpp"
#include "surface_cache.hpp"

namespace graphics
{

material::material() : shininess_(0)
{
}

void material::set_texture(const std::string& image)
{
	tex_ = texture::get(surface_cache::get(image));
}

void material::set_as_current_material() const
{
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emission_.array());
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient_.array());
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse_.array());
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular_.array());
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess_);
	tex_.set_as_current_texture();
}

}
