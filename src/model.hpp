
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef MODEL_HPP_INCLUDED
#define MODEL_HPP_INCLUDED

#include <GL/glew.h>
#include <boost/array.hpp>
#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include "eigen/projective.h"

#include "material.hpp"
#include "model_fwd.hpp"

namespace graphics
{

class model
{
public:
	const std::string& id() const { return id_; }

	static const_model_ptr get_model(const std::string& key);
	struct bone;

	struct vertex {
		vertex() : uvmap_valid(false) { influences.push_back(std::make_pair(-1,1.0)); }
		boost::array<GLfloat,3> point;
		boost::array<GLfloat,3> normal;
		boost::array<GLfloat,2> uvmap;
		bool uvmap_valid;
		std::vector<std::pair<int, float> > influences;
	};
	typedef boost::shared_ptr<vertex> vertex_ptr;

	struct face {
		GLenum primitive_type;
		face() { primitive_type = GL_TRIANGLES; transform.loadIdentity(); }
		std::vector<vertex_ptr> vertices;
		std::string material_name;
		const_material_ptr mat;
		Eigen::MatrixP3f transform;
	};

	struct bone {
		bone() : parent(-1) {}
		std::string name;
		int parent;
		boost::array<GLfloat,3> default_pos;
		boost::array<GLfloat,3> default_rot;
	};

	explicit model(const std::vector<face>& faces);
	model(const std::vector<face>& faces, const std::vector<bone>& bones);
	~model();

	void draw(const const_material_ptr& mat = const_material_ptr()) const;

	void get_materials(std::vector<const_material_ptr>* mats) const;

private:
	void optimize();
	void init_normals();
	std::string id_;
	boost::array<GLfloat,3> face_normal(const face& f, const vertex_ptr& v) const;
	boost::array<GLfloat,3> face_normal(const face& f, int n) const;
	std::vector<face> faces_;
	std::vector<bone> bones_;

	void update_arrays();
	GLfloat* vertex_array;
	GLfloat* normal_array;
	GLfloat* texcoord_array;
	unsigned int* element_array;
	
	GLuint vertex_buffer_objects[4];
};

}

#endif
