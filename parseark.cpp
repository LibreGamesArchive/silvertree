
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "model.hpp"
#include "parseark.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

namespace graphics {

namespace {

const std::string lbrace = "{";
const std::string rbrace = "}";

bool get_next_token(const char*& i1, const char* i2, std::string* token)
{
	while(i1 != i2 && isspace(*i1)) {
		++i1;
	}

	const char* beg = i1;

	while(i1 != i2 && !isspace(*i1)) {
		++i1;
	}

	if(i1 != i2 && i1+1 != i2 && i1[0] == '/' && i1[1] == '/') {
		i1 = std::find(i1,i2,'\n');
		return get_next_token(i1,i2,token);
	}

	token->assign(beg,i1);
	return !token->empty();
}

void expect_token(const char*& i1, const char* i2, const std::string& token)
{
	std::string str;
	if(!get_next_token(i1,i2,&str) || str != token) {
		throw parseark_error();
	}
}

void strip_string(std::string* str)
{
	str->erase(std::remove(str->begin(),str->end(),'"'),str->end());
}

void parse_mesh(const char*& i1, const char* i2, std::vector<graphics::model::face>& res, const std::vector<const_material_ptr>& mats)
{
	const_material_ptr mat;
	std::cerr << "parse mesh...\n";
	std::vector<model::vertex_ptr> vertices;
	std::string token;
	while(get_next_token(i1, i2, &token) && token != rbrace) {
		if(token == "VertexBuffer") {
			while(get_next_token(i1, i2, &token) && token != lbrace) {
			}

			std::vector<GLfloat> nums;
			while(get_next_token(i1, i2, &token) && token != rbrace) {
				nums.push_back(strtof(token.c_str(), NULL));
			}

			std::vector<GLfloat>::const_iterator i = nums.begin();
			while(i <= nums.end()-8) {
				model::vertex_ptr v(new model::vertex());
				v->point[0] = *i++/100.0;
				v->point[1] = *i++/100.0;
				v->point[2] = *i++/100.0;
				v->normal[0] = *i++/100.0;
				v->normal[1] = *i++/100.0;
				v->normal[2] = *i++/100.0;
				v->uvmap[0] = *i++;
				v->uvmap[1] = *i++;
				v->uvmap_valid = true;
				vertices.push_back(v);
			}
		} else if(token == "BoneBindings") {
			std::cerr << token << "\n";
			while(get_next_token(i1, i2, &token) && token != lbrace) {
			}

			int i = 0;
			while(get_next_token(i1, i2, &token) && token != rbrace) {
				if(i < vertices.size()) {
					vertices[i]->bone_num = strtol(token.c_str(), NULL, 10);
				}
				++i;
			}
		} else if(token == "Mesh") {
			std::cerr << token << "\n";
			get_next_token(i1, i2, &token);

			const unsigned int index = strtoul(token.c_str(),NULL,10);
			if(index < mats.size()) {
				mat = mats[index];
			}

			while(get_next_token(i1, i2, &token) && token != lbrace) {
			}

			while(get_next_token(i1, i2, &token) && token != rbrace) {
				if(token == "Triangle") {
					get_next_token(i1, i2, &token);
					const bool is_strip = token == "strip";
					while(get_next_token(i1, i2, &token) && token != lbrace) {
					}

					std::vector<unsigned int> t;
					while(get_next_token(i1, i2, &token) && token != rbrace) {
						t.push_back(strtoul(token.c_str(),NULL,10));
					}

					if(is_strip) {
						res.push_back(model::face());
						model::face& f = res.back();
						f.mat = mat;
						for(std::vector<unsigned int>::const_iterator i = t.begin();
						    i != t.end(); ++i) {
							if(*i < vertices.size()) {
								f.vertices.push_back(vertices[*i]);
							}
						}
					} else {
						std::vector<unsigned int>::const_iterator i = t.begin();
						while(i <= t.end()-3) {
							res.push_back(model::face());
							model::face& f = res.back();
							f.mat = mat;
							for(int n = 0; n != 3; ++n) {
								unsigned int index = *i++;
								if(index < vertices.size()) {
									f.vertices.push_back(vertices[index]);
								}
							}
						}
					}
				}
			}
		}
	}
}

}

model_ptr parseark(const char* i1, const char* i2)
{
	const GLfloat material_data[] = {1.0,1.0,1.0,1.0};

	std::vector<const_material_ptr> materials;
	std::vector<model::face> faces;
	std::vector<model::bone> bones;
	std::string token;
	while(get_next_token(i1, i2, &token)) {
		if(token == "Materials") {
			expect_token(i1, i2, lbrace);
			while(get_next_token(i1, i2, &token) && token != rbrace) {
				material_ptr m(new material);
				m->set_texture(token);
				m->set_ambient(material_data);
				m->set_diffuse(material_data);
				m->set_specular(material_data);
				materials.push_back(m);
			}
		} else if(token == "SubModels") {
			std::cerr << token << "\n";
			while(get_next_token(i1, i2, &token) && token != lbrace) {
			}

			while(get_next_token(i1, i2, &token) && token != rbrace) {
				if(token == "SubModel") {
					std::cerr << token << "\n";
					parse_mesh(i1, i2, faces, materials);
				}
			}
		} else if(token == "Skeleton") {
			while(get_next_token(i1, i2, &token) && token != lbrace) {
			}

			std::multimap<std::string,int> parents;
			while(get_next_token(i1, i2, &token) && token != rbrace) {
				bones.push_back(model::bone());
				model::bone& b = bones.back();
				strip_string(&token);
				b.name = token;
				get_next_token(i1, i2, &token);
				b.default_pos[0] = strtof(token.c_str(),NULL)/100.0;
				get_next_token(i1, i2, &token);
				b.default_pos[1] = strtof(token.c_str(),NULL)/100.0;
				get_next_token(i1, i2, &token);
				b.default_pos[2] = strtof(token.c_str(),NULL)/100.0;
				get_next_token(i1, i2, &token);
				b.default_rot[0] = strtof(token.c_str(),NULL);
				get_next_token(i1, i2, &token);
				b.default_rot[1] = strtof(token.c_str(),NULL);
				get_next_token(i1, i2, &token);
				b.default_rot[2] = strtof(token.c_str(),NULL);
				get_next_token(i1, i2, &token);
				strip_string(&token);
				parents.insert(std::pair<std::string,int>(token,bones.size()-1));
			}

			for(unsigned int n = 0; n != bones.size(); ++n) {
				typedef std::multimap<std::string,int>::const_iterator Itor;
				std::pair<Itor,Itor> range = parents.equal_range(bones[n].name);
				while(range.first != range.second) {
					bones[range.first->second].parent = n;
					++range.first;
				}
			}
		}
	}

	std::cerr << "loaded model with " << faces.size() << "," << bones.size() << "\n";
	return model_ptr(new graphics::model(faces, bones));
}
		
}
