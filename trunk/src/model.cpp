
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

using Eigen::MatrixP3f;
using Eigen::Vector3f;

namespace
{
std::map<std::string,graphics::const_model_ptr> model_cache;
std::set<std::string> bad_models;
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

    const std::set<std::string>::const_iterator itor2 =
        bad_models.find(key);
    if(itor2 != bad_models.end()) {
        return const_model_ptr();
    }

	const std::string data = sys::read_file(key);
	if(data.empty()) {
        bad_models.insert(bad_models.begin(), key);
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
	res->id_ = key;
	model_cache.insert(std::pair<std::string,const_model_ptr>(key,res));
	return res;
}

model::model(const std::vector<model::face>& faces) :
	faces_(faces),
	vertex_array(NULL),
	normal_array(NULL),
	texcoord_array(NULL),
	element_array(NULL)
{
	//optimize();
	//init_normals();
	update_arrays();
}

model::model(const std::vector<model::face>& faces,
             const std::vector<model::bone>& bones)
  :
	faces_(faces),
	bones_(bones),
	vertex_array(NULL),
	normal_array(NULL),
	texcoord_array(NULL),
	element_array(NULL)
{
	//init_normals();
	update_arrays();
}

model::~model()
{
	if(vertex_array != NULL)
		delete[] vertex_array;
	if(normal_array != NULL)
		delete[] normal_array;
	if(texcoord_array != NULL)
		delete[] texcoord_array;
	if(element_array != NULL)
		delete[] element_array;

	if (GLEW_VERSION_1_5) {
		glDeleteBuffers(4, vertex_buffer_objects);
	}
}

void model::init_normals()
{
	std::set<vertex_ptr> already_done;
	foreach(face& f, faces_) {
		foreach(vertex_ptr& v, f.vertices) {
			if(already_done.count(v) != 0) {
				continue;
			}

			v->normal[0] = face_normal(f,v)[0];
			v->normal[1] = face_normal(f,v)[1];
			v->normal[2] = face_normal(f,v)[2];

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

void model::update_arrays()
{
	update_skinning_matrices();

	unsigned int num_vertices = 0;
	foreach(face& f, faces_) {
		num_vertices += f.vertices.size();
	}

	if(vertex_array == NULL) {
		vertex_array = new GLfloat[num_vertices*3];
	}
	if(normal_array == NULL) {
		normal_array = new GLfloat[num_vertices*3];
	}
	if(texcoord_array == NULL) {
		texcoord_array = new GLfloat[num_vertices*2];
	}
	if(element_array == NULL) {
		element_array = new unsigned int[num_vertices];
	}

	std::map<vertex_ptr, unsigned int> added_vertices;
	typedef std::map<vertex_ptr, unsigned int>::const_iterator added_vertex_iterator;

	unsigned int current_vertex = 0;
	unsigned int current_index = 0;

	foreach(const face& f, faces_) {
		foreach(const vertex_ptr& v, f.vertices) {
			added_vertex_iterator added_vertex = added_vertices.find(v);
			if(added_vertex == added_vertices.end()) {
				MatrixP3f skinning_matrix;
				skinning_matrix.loadZero();
				for(int i = 0; i < v->influences.size(); i++) {
					MatrixP3f bone_matrix;
					if(v->influences[i].first == -1)
						bone_matrix.loadIdentity();
					else
						bone_matrix = bones_[v->influences[i].first].skinning_matrix.matrix();
					skinning_matrix.matrix() += bone_matrix.matrix() * v->influences[i].second;
				}
				Eigen::Matrix3f transpose_normal_matrix = skinning_matrix.linearComponent().inverse();
				Eigen::Matrix3f normal_matrix;
				normal_matrix.readRows(transpose_normal_matrix.array());

				GLvoid* vertex_pos = vertex_array + current_vertex * 3;
				Vector3f vertex = skinning_matrix * v->point;
				memcpy(vertex_pos, vertex.array(), 3 * sizeof(GLfloat));
				GLvoid* normal_pos = normal_array + current_vertex * 3;
				Vector3f normal = normal_matrix * v->normal;
				normal.normalize();
				memcpy(normal_pos, normal.array(), 3 * sizeof(GLfloat));
				GLvoid* texcoord_pos = texcoord_array + current_vertex * 2;
				memcpy(texcoord_pos, v->uvmap.array(), 2 * sizeof(GLfloat));
				f.mat->set_coord_manual(((GLfloat*)texcoord_pos)[0], ((GLfloat*)texcoord_pos)[1]);
				*(element_array + current_index) = current_vertex;
				added_vertices[v] = current_vertex;
				current_index++;
				current_vertex++;
			}
			else {
				*(element_array + current_index) = added_vertex->second;
				current_index++;
			}
		}
	}

	if (GLEW_VERSION_1_5) {
		glGenBuffers(4, vertex_buffer_objects);

		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_objects[0]);
		glBufferData(GL_ARRAY_BUFFER, current_vertex * 3 * sizeof(GLfloat), vertex_array, GL_STATIC_DRAW);
		delete[] vertex_array; vertex_array = NULL;
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_objects[1]);
		glBufferData(GL_ARRAY_BUFFER, current_vertex * 3 * sizeof(GLfloat), normal_array, GL_STATIC_DRAW);
		delete[] normal_array; normal_array = NULL;
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_objects[2]);
		glBufferData(GL_ARRAY_BUFFER, current_vertex * 2 * sizeof(GLfloat), texcoord_array, GL_STATIC_DRAW);
		delete[] texcoord_array; texcoord_array = NULL;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer_objects[3]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, current_index * sizeof(unsigned int), element_array, GL_STATIC_DRAW);
		delete[] element_array; element_array = NULL;
	}
}

void model::update_skinning_matrices()
{
	foreach(bone& the_bone, bones_)
		the_bone.skinning_matrix_valid = false;

	foreach(bone& the_bone, bones_)
		the_bone.update_skinning_matrix(bones_);

	foreach(bone& the_bone, bones_)
		the_bone.skinning_matrix *= the_bone.inv_bind_matrix;
}

void model::bone::update_skinning_matrix(std::vector<bone>& bones)
{
	if(!skinning_matrix_valid) {
		if(parent == -1)
			skinning_matrix = transform;
		else {
			bones[parent].update_skinning_matrix(bones);
			skinning_matrix = bones[parent].skinning_matrix * transform;
		}
		skinning_matrix_valid = true;
	}
}

void model::draw(const const_material_ptr& mat) const
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	if (GLEW_VERSION_1_5) {
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_objects[0]);
	}
	glVertexPointer(3, GL_FLOAT, 0, vertex_array);
	if (GLEW_VERSION_1_5) {
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_objects[1]);
	}
	glNormalPointer(GL_FLOAT, 0, normal_array);
	if (GLEW_VERSION_1_5) {
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_objects[2]);
	}
	glTexCoordPointer(2, GL_FLOAT, 0, texcoord_array);

	if (GLEW_VERSION_1_5) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer_objects[3]);
	}
	glMatrixMode(GL_MODELVIEW);
	unsigned int* face_elements = element_array;
	foreach(const face& f, faces_) {
		unsigned int num_vertices = f.vertices.size();
		if(!mat || (mat == f.mat)) {
			glPushMatrix();
			glMultMatrixf(f.transform.array());
			f.mat->set_as_current_material();
			glDrawElements(f.primitive_type, num_vertices, GL_UNSIGNED_INT, face_elements);
			glPopMatrix();
		}
		face_elements += num_vertices;
	}

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
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
