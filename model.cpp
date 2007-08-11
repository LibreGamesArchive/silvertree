
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "filesystem.hpp"
#include "foreach.hpp"
#include "model.hpp"
#include "parse3ds.hpp"
#include "parseark.hpp"
#include "parsedae.hpp"

#include <map>
#include <math.h>
#include <set>

#include <iostream>

namespace
{
std::map<std::string,graphics::const_model_ptr> model_cache;	
const std::string ark_header = "ArkModel";
}

namespace graphics
{

const_model_ptr model::get_model(const std::string& key)
{
	const std::map<std::string,const_model_ptr>::const_iterator itor =
	                  model_cache.find(key);
	if(itor != model_cache.end()) {
		return itor->second;
	}

	const std::string data = sys::read_file(key);
	if(data.empty()) {
		return const_model_ptr();
	}

	model_ptr res;
	
	if(data.size() > ark_header.size() &&
	   !memcmp(data.data(), ark_header.data(), ark_header.size())) {
		res = parseark(data.c_str(),data.c_str()+data.size());
	} else if(data[0] == '<') {
		res = parsedae(data.c_str(),data.c_str()+data.size());
	} else {
		res = parse3ds(data.c_str(),data.c_str()+data.size());
	}
	model_cache.insert(std::pair<std::string,const_model_ptr>(key,res));
	return res;
}

model::model(const std::vector<model::face>& faces) : faces_(faces)
{
	//optimize();
	init_normals();
}

model::model(const std::vector<model::face>& faces,
             const std::vector<model::bone>& bones)
  : faces_(faces), bones_(bones)
{
	//optimize();
	init_normals();
}

namespace {
bool merge_faces(model::face& a, model::face& b)
{
	std::vector<model::vertex_ptr>& va = a.vertices;
	std::vector<model::vertex_ptr>& vb = b.vertices;
	if(va.size() < 3 || vb.size() != 3) {
		std::cerr << "bad sizes: " << va.size() << "," << vb.size() << "\n";
		return false;
	}

	model::vertex_ptr different;
	int unfound = 0;
	foreach(const model::vertex_ptr& vp, vb) {
		if(std::find(va.end()-3,va.end(),vp) == va.end()) {
			++unfound;
			different = vp;
		}
	}

	std::cerr << "unfound(1): " << unfound << "\n";

	if(unfound == 1) {
		va.push_back(different);
		return true;
	}

	unfound = 0;
	foreach(const model::vertex_ptr& vp, vb) {
		if(std::find(va.begin(),va.begin()+3,vp) == va.end()) {
			++unfound;
			different = vp;
		}
	}

	std::cerr << "unfound(2): " << unfound << "\n";

	if(unfound == 1) {
		va.insert(va.begin(),different);
		return true;
	}

	return false;
}

}

void model::optimize()
{
	std::cerr << "BEFORE OPTIMIZE: " << faces_.size() << "\n";
	for(int n = 0; n < faces_.size()-1; ) {
		if(merge_faces(faces_[n],faces_[n+1])) {
			faces_.erase(faces_.begin()+n+1);
		} else {
			++n;
		}
	}
	std::cerr << "AFTER OPTIMIZE: " << faces_.size() << "\n";
}

void model::init_normals()
{
	std::set<vertex_ptr> already_done;
	foreach(face& f, faces_) {
		foreach(vertex_ptr& v, f.vertices) {
			if(already_done.count(v) != 0) {
				continue;
			}

			v->normal = face_normal(f,v);

			/*
			foreach(const face& f,faces_) {
				if(std::count(f.vertices.begin(),f.vertices.end(),v)) {
					boost::array<GLfloat,3> normal = face_normal(f,v);
					for(unsigned int index = 0; index != 3; ++index) {
						v->normal[index] += normal[index];
					}
				}
			}
			*/

			already_done.insert(v);
		}
	}
}

boost::array<GLfloat,3> model::face_normal(const model::face& f, const model::vertex_ptr& v) const
{
	boost::array<GLfloat,3> res;
	std::fill(res.begin(),res.end(),0.0);

	std::vector<vertex_ptr>::const_iterator i = std::find(f.vertices.begin(),f.vertices.end(),v);
	if(i == f.vertices.end()) {
		return res;
	}

	int index = (i - f.vertices.begin()) - 2;
	for(int n = index; n != index+3; ++n) {
		if(n < 0 || n > f.vertices.size()-3) {
			continue;
		}

		boost::array<GLfloat,3> val = face_normal(f, n);
		for(int m = 0; m != 3; ++m) {
			res[m] += val[m];
		}
	}
	
	return res;
}

boost::array<GLfloat,3> model::face_normal(const model::face& f, int n) const
{
	boost::array<GLfloat,3> v;
	boost::array<GLfloat,3> w;

	v[0] = f.vertices[n+1]->point[0] - f.vertices[n+0]->point[0];
	v[1] = f.vertices[n+1]->point[1] - f.vertices[n+0]->point[1];
	v[2] = f.vertices[n+1]->point[2] - f.vertices[n+0]->point[2];

	w[0] = f.vertices[n+2]->point[0] - f.vertices[n+1]->point[0];
	w[1] = f.vertices[n+2]->point[1] - f.vertices[n+1]->point[1];
	w[2] = f.vertices[n+2]->point[2] - f.vertices[n+1]->point[2];

	boost::array<GLfloat,3> res;
	res[0] = v[1]*w[2] - v[2]*w[1];
	res[1] = v[0]*w[2] - v[2]*w[0];
	res[2] = v[1]*w[2] - v[2]*w[1];

	return res;
}

void model::draw() const
{
	bool in_triangles = false;
	foreach(const face& f, faces_) {
		f.mat->set_as_current_material();
		draw_face(f, in_triangles);
	}

	if(in_triangles) {
		glEnd();
	}
}

void model::draw_material(const const_material_ptr& mat) const
{
	bool in_triangles = false;
	foreach(const face& f, faces_) {
		if(mat == f.mat) {
			draw_face(f, in_triangles);
		}
	}

	if(in_triangles) {
		glEnd();
	}
}

namespace {

GLfloat deg_to_rad(GLfloat angle)
{
	return (angle*3.14159)/180.0;
}

void roll_matrix(GLfloat angle, GLfloat* matrix)
{
	angle = deg_to_rad(angle);
	matrix[0 + 0*3] += 1.0;
	matrix[1 + 0*3] += 0.0;
	matrix[2 + 0*3] += 0.0;
	matrix[0 + 1*3] += 0.0;
	matrix[1 + 1*3] += cos(angle);
	matrix[2 + 1*3] += sin(angle);
	matrix[0 + 2*3] += 0.0;
	matrix[1 + 2*3] += -sin(angle);
	matrix[2 + 2*3] += cos(angle);
}

void pitch_matrix(GLfloat angle, GLfloat* matrix)
{
	angle = deg_to_rad(angle);
	matrix[0 + 0*3] += cos(angle);
	matrix[1 + 0*3] += 0.0;
	matrix[2 + 0*3] += -sin(angle);
	matrix[0 + 1*3] += 0.0;
	matrix[1 + 1*3] += 1.0;
	matrix[2 + 1*3] += 0.0;
	matrix[0 + 2*3] += sin(angle);
	matrix[1 + 2*3] += 0.0;
	matrix[2 + 2*3] += cos(angle);
}

void yaw_matrix(GLfloat angle, GLfloat* matrix)
{
	angle = deg_to_rad(angle);
	matrix[0 + 0*3] += cos(angle);
	matrix[1 + 0*3] += sin(angle);
	matrix[2 + 0*3] += 0.0;
	matrix[0 + 1*3] += -sin(angle);
	matrix[1 + 1*3] += cos(angle);
	matrix[2 + 1*3] += 0.0;
	matrix[0 + 2*3] += 0.0;
	matrix[1 + 2*3] += 0.0;
	matrix[2 + 2*3] += 1.0;
}

void mult_matrix(const GLfloat* matrix, GLfloat* vec)
{
	GLfloat res[3];
	res[0] = matrix[0 + 0*3]*vec[0] + matrix[0 + 1*3]*vec[1] + matrix[0 + 2*3]*vec[2];
	res[1] = matrix[1 + 0*3]*vec[0]  + matrix[1 + 1*3]*vec[1] + matrix[1 + 2*3]*vec[2];
	res[2] = matrix[2 + 0*3]*vec[0]  + matrix[2 + 1*3]*vec[1] + matrix[2 + 2*3]*vec[2];
	vec[0] = res[0];
	vec[1] = res[1];
	vec[2] = res[2];
}
		
}

void model::draw_face(const face& f, bool& in_triangles) const
{
	if(f.vertices.size() > 3) {
		if(in_triangles) {
			glEnd();
		}

		glBegin(GL_TRIANGLE_STRIP);
		in_triangles = false;
	} else if(!in_triangles) {
		glBegin(GL_TRIANGLES);
		in_triangles = true;
	}

	foreach(const vertex_ptr& v,f.vertices) {
		if(v->uvmap_valid) {
			graphics::texture::set_coord(v->uvmap[0],v->uvmap[1]);
		}

		GLfloat vertex[] = {v->point[0],v->point[1],v->point[2]};
		GLfloat normal[] = {v->normal[0],v->normal[1],v->normal[2]};

		int bones[64] = {v->bone_num};
		int cur_bone = 0;
		while(bones[cur_bone] != -1) {
			bones[cur_bone+1] = bones_[bones[cur_bone]].parent;
			++cur_bone;
		}

		for(int n = 0; n != cur_bone; ++n) {
			const bone& b = bones_[bones[n]];

			vertex[0] += b.default_pos[0];
			vertex[1] += b.default_pos[1];
			vertex[2] += b.default_pos[2];

			GLfloat matrix[9] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
			roll_matrix(b.default_rot[0], matrix);
			pitch_matrix(b.default_rot[1], matrix);
			yaw_matrix(b.default_rot[2], matrix);
			mult_matrix(matrix, vertex);
			mult_matrix(matrix, normal);
		}

		glNormal3fv(normal);
		glVertex3fv(vertex);
	}

	if(f.vertices.size() > 3) {
		glEnd();
	}
}

void model::get_materials(std::vector<const_material_ptr>* mats) const
{
	foreach(const face& f, faces_) {
		if(std::find(mats->begin(),mats->end(),f.mat) == mats->end()) {
			mats->push_back(f.mat);
		}
	}
}

}
