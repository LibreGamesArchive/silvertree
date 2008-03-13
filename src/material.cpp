
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

void material::set_emission(const GLfloat* v)
{
	std::copy(v,v+3,emission_.begin());
	emission_[3] = 1.0;
}

void material::set_ambient(const GLfloat* v)
{
	std::copy(v,v+3,ambient_.begin());
	ambient_[3] = 1.0;
}

void material::set_diffuse(const GLfloat* v)
{
	std::copy(v,v+3,diffuse_.begin());
	diffuse_[3] = 1.0;
}

void material::set_specular(const GLfloat* v)
{
	std::copy(v,v+3,specular_.begin());
	specular_[3] = 1.0;
}

void material::set_shininess(GLfloat shininess)
{
	shininess_ = shininess;
}

void material::set_as_current_material() const
{
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,emission_.data());
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient_.data());
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse_.data());
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular_.data());
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess_);
	tex_.set_as_current_texture();
}

}
