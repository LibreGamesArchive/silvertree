
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "foreach.hpp"
#include "model.hpp"
#include "parseark.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
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
    
    if(beg != i2 && beg+1 != i2 && beg[0] == '/' && beg[1] == '/') {
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

void parse_mesh(const char*& i1, const char* i2, 
                std::vector<graphics::model::face>& res, 
                const std::vector<const_material_ptr>& mats)
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
                v->normal[0] = *i++;
                v->normal[1] = *i++;
                v->normal[2] = *i++;
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
                vertices[i]->influences.clear();
                if(i < vertices.size()) {
                    vertices[i]->influences.push_back(std::make_pair(strtol(token.c_str(), NULL, 10), 1.0));
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

                    res.push_back(model::face());
                    model::face& f = res.back();
                    if(token == "strip") {
                        f.primitive_type = GL_TRIANGLE_STRIP;
                    } else if(token == "block") {
                        f.primitive_type = GL_TRIANGLES;
                    } else if(token == "fan") {
                        f.primitive_type = GL_TRIANGLE_FAN;
                    }
                    while(get_next_token(i1, i2, &token) && token != lbrace) {
                    }
                    
                    std::vector<unsigned int> t;
                    while(get_next_token(i1, i2, &token) && token != rbrace) {
                        t.push_back(strtoul(token.c_str(),NULL,10));
                    }
                    
                    f.mat = mat;

                    for(std::vector<unsigned int>::const_iterator i = t.begin();
                        i != t.end(); ++i) {
                        if(*i < vertices.size()) {
                            f.vertices.push_back(vertices[*i]);
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
    const GLfloat material_data_specular[] = {0.0,0.0,0.0,1.0};
    
    std::vector<const_material_ptr> materials;
    std::vector<model::face> faces;
    std::vector<model::bone> bones;
    std::string token;
    Eigen::MatrixP3f rootMatrix;

    rootMatrix.loadIdentity();

    while(get_next_token(i1, i2, &token)) {
        if(token == "Materials") {
            expect_token(i1, i2, lbrace);
            while(get_next_token(i1, i2, &token) && token != rbrace) {
                material_ptr m(new material);
                m->set_texture(token);
                m->set_emission(material_data_specular);
                m->set_ambient(material_data);
                m->set_diffuse(material_data);
                m->set_specular(material_data_specular);
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
        } else if(token == "FixXRot") {
            const static Eigen::Vector3f x_axis(1,0,0);
            const static Eigen::Vector3f y_axis(0,1,0);
            const static Eigen::Vector3f z_axis(0,0,1);
            
            rootMatrix.rotate3(-M_PI/2, x_axis);
            rootMatrix.rotate3(M_PI/2, y_axis);
        } else if(token == "Skeleton") {
            while(get_next_token(i1, i2, &token) && token != lbrace) {
            }

            std::multimap<std::string,int> parents;
            while(get_next_token(i1, i2, &token) && token != rbrace) {
                const static GLfloat deg_to_rad = M_PI/180.0;

                bones.push_back(model::bone());
                model::bone& b = bones.back();
                Eigen::Vector3f default_pos;
                strip_string(&token);
                b.name = token;

                get_next_token(i1, i2, &token);
                default_pos[0] = strtof(token.c_str(),NULL)/100.0;
                get_next_token(i1, i2, &token);
                default_pos[1] = strtof(token.c_str(),NULL)/100.0;
                get_next_token(i1, i2, &token);
                default_pos[2] = strtof(token.c_str(),NULL)/100.0;
                get_next_token(i1, i2, &token);
                GLfloat z1 = strtof(token.c_str(),NULL) * deg_to_rad/2;
                get_next_token(i1, i2, &token);
                GLfloat z2 = strtof(token.c_str(),NULL) * deg_to_rad/2;
                get_next_token(i1, i2, &token);
                GLfloat z3 = strtof(token.c_str(),NULL) * deg_to_rad/2;
                get_next_token(i1, i2, &token);

                strip_string(&token);
                std::string parent = token;

                Eigen::MatrixP3f default_matrix;
                default_matrix.loadIdentity();
                default_matrix.setTranslationVector(default_pos);

                // TODO: pull out into quaternion class
                const GLfloat sz1 = sinf(z1);
                const GLfloat sz2 = sinf(z2);
                const GLfloat sz3 = sinf(z3);
                const GLfloat cz1 = cosf(z1);
                const GLfloat cz2 = cosf(z2);
                const GLfloat cz3 = cosf(z3);

                const GLfloat q_w = - (sz1*sz2*sz3 + cz1*cz2*cz3);
                const GLfloat q_x = sz1*cz2*cz3 - cz1*sz2*sz3;
                const GLfloat q_y = cz1*sz2*cz3 + sz1*cz2*sz3;
                const GLfloat q_z = cz1*cz2*sz3 - sz1*sz2*cz3;
                                
                default_matrix(0,0) = 1-2*q_y*q_y - 2*q_z*q_z;
                default_matrix(0,1) = 2*q_x*q_y - 2*q_w*q_z;
                default_matrix(0,2) = 2*q_z*q_x + 2*q_w*q_y;
                default_matrix(1,0) = 2*q_x*q_y + 2*q_w*q_z;
                default_matrix(1,1) = 1-2*q_x*q_x - 2*q_z*q_z;
                default_matrix(1,2) = 2*q_y*q_z - 2*q_w*q_x;
                default_matrix(2,0) = 2*q_z*q_x - 2*q_w*q_y;
                default_matrix(2,1) = 2*q_y*q_z + 2*q_w*q_x;
                default_matrix(2,2) = 1 - 2*q_x*q_x - 2*q_y*q_y;
                // end of quaternion horror

                b.transform = default_matrix.matrix();
                b.inv_bind_matrix = default_matrix.matrix();
                
                parents.insert(std::pair<std::string,int>(parent,bones.size()-1));
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

    foreach(model::bone& b, bones) {
        int parent = b.parent;
        while(parent != -1) {
            b.inv_bind_matrix = bones[parent].transform * b.transform;
            parent = bones[parent].parent;
        }
        b.inv_bind_matrix.matrix().inverse();
        b.inv_bind_matrix *= rootMatrix;
    }

    std::cout << "loaded model with " << faces.size() << "," << bones.size() << "\n";
    return model_ptr(new graphics::model(faces, bones));
}

}
