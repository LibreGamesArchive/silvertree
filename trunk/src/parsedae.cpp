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

#include "boost/array.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "model.hpp"
#include "parsedae.hpp"
#include "tinyxml/tinyxml.h"

#include <iostream>
#include <sstream>
#include <map>

using std::vector;
using std::map;
using std::string;
using std::istringstream;

namespace graphics
{

namespace {
const GLfloat ScaleFactor = 0.009;
}

void parse4vector(const string& str, GLfloat* v);

class COLLADA
{
	TiXmlDocument doc_;
	const TiXmlElement* root_;
	map<string,const TiXmlElement*> ids_;

	vector<std::pair<string, GLenum> > primitive_types;

	const TiXmlElement* resolve_shorthand_ptr(string ptr) const;
	const vector<model::face> get_faces_from_geometry(const TiXmlElement*) const;
	const vector<model::face> get_faces_from_node(const TiXmlElement*) const;
	const_material_ptr get_material(const TiXmlElement*) const;
	template<typename T> vector<T> get_array(const TiXmlElement*) const;

	public:
	explicit COLLADA(const char*);
	const vector<model::face> get_faces() const;
};

COLLADA::COLLADA(const char* i1)
{
	primitive_types.push_back(std::make_pair(string("triangles"), GL_TRIANGLES));
	primitive_types.push_back(std::make_pair(string("tristrips"), GL_TRIANGLE_STRIP));

	doc_.Parse(i1);
	if(doc_.Error())
		throw parsedae_error(string("Parse error: ") + doc_.ErrorDesc());

	root_ = doc_.RootElement();
	if(root_->Value() != string("COLLADA"))
		throw parsedae_error("Not a COLLADA document.");

	struct id_collector_t : public TiXmlVisitor
	{
		map<string,const TiXmlElement*>& ids_;
		id_collector_t(map<string,const TiXmlElement*>& ids) 
			: ids_(ids) {}
		bool VisitEnter(const TiXmlElement& element, const TiXmlAttribute* attribute)
		{
			const char* id = element.Attribute("id");
			if(id) {
				ids_[id] = &element;
			}
			return true;
		}
	} id_collector(ids_);
	root_->Accept(&id_collector);
}

const TiXmlElement* COLLADA::resolve_shorthand_ptr(string id) const
{
	if(id[0] != '#') return NULL;
	id.erase(id.begin());
	return (*(ids_.find(id))).second;
}

const vector<model::face> COLLADA::get_faces() const
{
	vector<model::face> faces;
	const TiXmlElement* scene_instance =
		TiXmlHandle(const_cast<TiXmlElement*>(root_)).FirstChild("scene").FirstChild("instance_visual_scene").ToElement();
	if(scene_instance) {
		const TiXmlElement* visual_scene = resolve_shorthand_ptr(scene_instance->Attribute("url"));
		const TiXmlElement* node = visual_scene->FirstChildElement("node");
		for(; node; node = node->NextSiblingElement("node")) {
			vector<model::face> node_faces = get_faces_from_node(node);
			faces.insert(faces.end(), node_faces.begin(), node_faces.end());
		}
	}
	return faces;
}

const vector<model::face> COLLADA::get_faces_from_node(const TiXmlElement* node) const
{
	vector<model::face> faces;
	const TiXmlElement* geometry = node->FirstChildElement("instance_geometry");
	for(; geometry; geometry = geometry->NextSiblingElement("instance_geometry")) {
		vector<model::face> geometry_faces =
			get_faces_from_geometry(resolve_shorthand_ptr(geometry->Attribute("url")));
		const TiXmlElement* material_instance =
			TiXmlHandle(const_cast<TiXmlElement*>(geometry)).FirstChild("bind_material").FirstChild("technique_common").FirstChild("instance_material").ToElement();
		for(; material_instance; material_instance = material_instance->NextSiblingElement("instance_material")) {
			const_material_ptr material = get_material(resolve_shorthand_ptr(material_instance->Attribute("target")));
			foreach(model::face& face, geometry_faces) {
				if(face.material_name == string(material_instance->Attribute("symbol")))
					face.mat = material;
			}
		}
		faces.insert(faces.end(), geometry_faces.begin(), geometry_faces.end());
	}
	const TiXmlElement* subnode = node->FirstChildElement("node");
	for(; subnode; subnode = subnode->NextSiblingElement("node")) {
		const vector<model::face> subnode_faces = get_faces_from_node(subnode);
		faces.insert(faces.end(), subnode_faces.begin(), subnode_faces.end());
	}
	return faces;
}

const vector<model::face> COLLADA::get_faces_from_geometry(const TiXmlElement* geometry) const
{
	vector<model::face> faces;
	const TiXmlElement* mesh = geometry->FirstChildElement("mesh");
	if(mesh) {
		vector<const TiXmlElement*> primitive_elements;
		typedef std::pair<string, GLenum> primitive_type_t;
		foreach(primitive_type_t primitive_type, primitive_types) {
			const TiXmlElement* primitive_element = mesh->FirstChildElement(primitive_type.first);
			for(; primitive_element; primitive_element = primitive_element->NextSiblingElement(primitive_type.first)) {
				int offset, max_offset = 0;
				bool have_positions = false;int positions_offset = 0;
				vector<GLfloat> positions;
				bool have_normals = false;int normals_offset = 0;
				vector<GLfloat> normals;
				bool have_texcoords = false;int texcoords_offset = 0;
				vector<GLfloat> texcoords;

				const TiXmlElement* input = primitive_element->FirstChildElement("input");
				for(; input; input = input->NextSiblingElement("input")) {
					input->QueryIntAttribute("offset", &offset);
					if(offset > max_offset) max_offset = offset;
					if(input->Attribute("semantic") == string("VERTEX")) {
						const TiXmlElement* vertices;
						vertices = resolve_shorthand_ptr(input->Attribute("source"));
						const TiXmlElement* positions_source =
							resolve_shorthand_ptr(vertices->FirstChildElement("input")->Attribute("source"));
						positions = get_array<GLfloat>(positions_source->FirstChildElement("float_array"));
						have_positions = true;
						positions_offset = offset;
					}
					if(input->Attribute("semantic") == string("NORMAL")) {
						const TiXmlElement* normals_source = resolve_shorthand_ptr(input->Attribute("source"));
						normals = get_array<GLfloat>(normals_source->FirstChildElement("float_array"));
						have_normals = true;
						normals_offset = offset;
					}
					if(input->Attribute("semantic") == string("TEXCOORD")) {
						const TiXmlElement* texcoords_source = resolve_shorthand_ptr(input->Attribute("source"));
						texcoords = get_array<GLfloat>(texcoords_source->FirstChildElement("float_array"));
						have_texcoords = true;
						texcoords_offset = offset;
					}
				}

				if(have_positions) {
					vector<int> primitive_indices = get_array<int>(primitive_element->FirstChildElement("p"));
					faces.push_back(model::face());
					model::face& face = faces.back();
					const char* material_name = primitive_element->Attribute("material");
					face.primitive_type = primitive_type.second;
					material_ptr mat(new material);
					GLfloat material_data[] = {1.0,1.0,1.0,1.0};
					mat->set_ambient(material_data);
					mat->set_diffuse(material_data);
					mat->set_specular(material_data);
					face.mat = mat;
					if(material_name)
						face.material_name = material_name;
					for(int i = 0; i < primitive_indices.size(); i += (max_offset+1)) {
						face.vertices.push_back(model::vertex_ptr(new model::vertex));
						model::vertex_ptr vertex = face.vertices.back();
						vertex->point[0] = positions[primitive_indices[i + positions_offset]*3] * ScaleFactor;
						vertex->point[1] = positions[primitive_indices[i + positions_offset]*3 + 1] * ScaleFactor;
						vertex->point[2] = positions[primitive_indices[i + positions_offset]*3 + 2] * ScaleFactor;
						if(have_normals) {
							vertex->normal[0] = -normals[primitive_indices[i + normals_offset]*3];
							vertex->normal[1] = -normals[primitive_indices[i + normals_offset]*3 + 1];
							vertex->normal[2] = -normals[primitive_indices[i + normals_offset]*3 + 2];
						}
						if(have_texcoords) {
							vertex->uvmap[0] =  texcoords[primitive_indices[i + texcoords_offset]*2];
							vertex->uvmap[1] = -texcoords[primitive_indices[i + texcoords_offset]*2 + 1];
							vertex->uvmap_valid = true;
						}
					}
				}
			}
		}
	}
	return faces;
}

template<typename T> vector<T> COLLADA::get_array(const TiXmlElement* array_element) const
{
	istringstream is(array_element->GetText());
	vector<T> items;
	while(is.good()) {
		T item;
		is >> item;
		items.push_back(item);
	}
	return items;
}

const_material_ptr COLLADA::get_material(const TiXmlElement* material_element) const
{
	material_ptr mat(new material);
	GLfloat material_data[] = {1.0,1.0,1.0,1.0};
	mat->set_ambient(material_data);
	mat->set_diffuse(material_data);
	mat->set_specular(material_data);
	bool have_texture = false;
	const char* effect_url = material_element->FirstChildElement("instance_effect")->Attribute("url");
	const TiXmlElement* effect_element = resolve_shorthand_ptr(effect_url);
	const TiXmlElement* newparam = effect_element->FirstChildElement("profile_COMMON")->FirstChildElement("newparam");
	const TiXmlElement* technique = effect_element->FirstChildElement("profile_COMMON")->FirstChildElement("technique");
	for(;newparam;newparam = newparam->NextSiblingElement("newparam")) {
		const TiXmlElement* surface = newparam->FirstChildElement("surface");
		if(surface && surface->Attribute("type") == string("2D")) {
			const TiXmlElement* image = surface->FirstChildElement("init_from");
			if(image) {
				image = (*(ids_.find(image->GetText()))).second;
				image = image->FirstChildElement("init_from");
				if(image) {
					mat->set_texture(basename(image->GetText()));
					have_texture = true;
				}
			}
		}
	}
	if(technique && !have_texture) {
		const TiXmlElement* phong = technique->FirstChildElement("phong");
		if(phong) {
			const TiXmlElement* color = phong->FirstChildElement();
			const TiXmlElement* color_value;
			for(;color;color = color->NextSiblingElement()) {
				if(color->Value() == string("ambient")) {
					color_value = color->FirstChildElement("color");
					if(color_value) {
						parse4vector(color_value->GetText(), material_data);
						mat->set_ambient(material_data);
					}
				}
				if(color->Value() == string("diffuse")) {
					color_value = color->FirstChildElement("color");
					if(color_value) {
						parse4vector(color_value->GetText(), material_data);
						mat->set_diffuse(material_data);
					}
				}
				if(color->Value() == string("specular")) {
					color_value = color->FirstChildElement("color");
					if(color_value) {
						parse4vector(color_value->GetText(), material_data);
						mat->set_specular(material_data);
					}
				}
			}
		}
	}
	return mat;
}

void parse4vector(const string& str, GLfloat* v)
{
	istringstream is(str);
	for(int i = 0; i < 4; i++) {
		GLfloat num;
		is >> num;
		v[i] = num;
	}
}

model_ptr parsedae(const char* i1, const char* i2)
{
	std::cerr << "Parsing COLLADA...\n";
	COLLADA collada(i1);
	return(model_ptr(new model(collada.get_faces())));
}

}
