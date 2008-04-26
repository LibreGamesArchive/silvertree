
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef MATERIAL_HPP_INCLUDED
#define MATERIAL_HPP_INCLUDED

#include <string>

#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>

#include "texture.hpp"

#include "eigen/vector.h"

namespace graphics
{

class material
{
public:
	material();

	void set_texture(const std::string& fname);
	void set_emission(const Eigen::Vector4f& v) { emission_ = v; }
	void set_ambient(const Eigen::Vector4f& v) { ambient_ = v; }
	void set_diffuse(const Eigen::Vector4f& v) { diffuse_ = v; }
	void set_specular(const Eigen::Vector4f& v) { specular_ = v; }
	void set_shininess(GLfloat shininess) { shininess_ = shininess; }

	void set_as_current_material() const;
	void set_coord(GLfloat x, GLfloat y) const { tex_.set_coord(x,y); }
	void set_coord_manual(GLfloat &x, GLfloat &y) const { tex_.set_coord_manual(x,y); }
private:
	texture tex_;

	Eigen::Vector4f ambient_, diffuse_, specular_, emission_;
	GLfloat shininess_;
};

typedef boost::shared_ptr<material> material_ptr;
typedef boost::shared_ptr<const material> const_material_ptr;

}

#endif
