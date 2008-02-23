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
#include "boost/tuple/tuple.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
#include "model.hpp"
#include "parsedae.hpp"
#include "tinyxml/tinyxml.h"
#include "eigen/projective.h"

#include <iostream>
#include <sstream>
#include <map>

#include <libgen.h>

using std::vector;
using std::map;
using std::multimap;
using std::string;
using std::istringstream;
using std::pair;
using std::make_pair;
using boost::tie;
using Eigen::Vector3f;
using Eigen::Vector4f;
using Eigen::MatrixP3f;
using Eigen::affToProj;

namespace graphics
{

namespace {
const GLfloat ScaleFactor = 0.009;

template <typename T, int n> std::istream& operator>>(std::istream& is, Eigen::Vector<T,n>& vec)
{
	for(int i = 0; i < n; i++)
		is >> vec[i];
	return is;
}

template <typename T, int n> std::istream& operator>>(std::istream& is, Eigen::MatrixP<T,n>& matrix)
{
	for(int row = 0; row < n+1; row++) {
		for(int col = 0; col < n+1; col++) {
			is >> matrix(row, col);
		}
	}
	return is;
}

void scale_translation_vector(MatrixP3f& mat)
{
	Vector3f vec;
	mat.getTranslationVector(&vec);
	vec *= ScaleFactor;
	mat.setTranslationVector(vec);
}

}

void parse4vector(const string& str, GLfloat* v);

class COLLADA
{
	TiXmlDocument doc_;
	const TiXmlElement* root_;
	map<string,const TiXmlElement*> ids_;

	vector<std::pair<string, GLenum> > primitive_types;

	const TiXmlElement* resolve_shorthand_ptr(string ptr) const;
	void get_transform_from_node(const TiXmlElement*, MatrixP3f&) const;
	pair<vector<model::face>, multimap<int, model::vertex_ptr> > get_faces_from_geometry(const TiXmlElement*) const;
	pair<vector<model::face>, vector<model::bone> > get_faces_and_bones_from_node(const TiXmlElement*) const;
	pair<vector<model::face>, vector<model::bone> > get_faces_and_bones_from_controller(const TiXmlElement*) const;
	int get_bones_from_skeleton(const TiXmlElement*, vector<model::bone>&) const;
	void bind_materials(const TiXmlElement*, vector<model::face>& faces) const;
	const_material_ptr get_material(const TiXmlElement*) const;
	template<typename T> vector<T> get_array(const TiXmlElement*) const;

	public:
	explicit COLLADA(const char*);
	pair<vector<model::face>, vector<model::bone> > get_faces_and_bones() const;
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
	if(id[0] != '#')
		throw parsedae_error(string("Unable to resolve url: ") + id + ". Only shorthand pointers are supported.");
	id.erase(id.begin());
	map<string,const TiXmlElement*>::const_iterator id_iter = ids_.find(id);
	if(id_iter == ids_.end())
		throw parsedae_error(string("Unable to resolve shorthand pointer: #") + id);
	else
		return id_iter->second;
}

pair<vector<model::face>, vector<model::bone> > COLLADA::get_faces_and_bones() const
{
	vector<model::face> faces;
	vector<model::bone> bones;
	const TiXmlElement* scene_instance =
		TiXmlHandle(const_cast<TiXmlElement*>(root_)).FirstChild("scene").FirstChild("instance_visual_scene").ToElement();
	if(scene_instance) {
		const TiXmlElement* visual_scene = resolve_shorthand_ptr(scene_instance->Attribute("url"));
		const TiXmlElement* node = visual_scene->FirstChildElement("node");
		for(; node; node = node->NextSiblingElement("node")) {
			vector<model::face> node_faces;
			vector<model::bone> node_bones;
			tie(node_faces, node_bones) = get_faces_and_bones_from_node(node);
			faces.insert(faces.end(), node_faces.begin(), node_faces.end());
			bones.insert(bones.end(), node_bones.begin(), node_bones.end());
		}
	}

	return make_pair(faces, bones);
}

pair<vector<model::face>, vector<model::bone> > COLLADA::get_faces_and_bones_from_node(const TiXmlElement* node) const
{
	vector<model::face> faces;
	vector<model::bone> bones;
	MatrixP3f transform;
	get_transform_from_node(node, transform);

	const TiXmlElement* geometry = node->FirstChildElement("instance_geometry");
	for(; geometry; geometry = geometry->NextSiblingElement("instance_geometry")) {
		vector<model::face> geometry_faces =
			get_faces_from_geometry(resolve_shorthand_ptr(geometry->Attribute("url"))).first;
		bind_materials(geometry, geometry_faces);
		faces.insert(faces.end(), geometry_faces.begin(), geometry_faces.end());
	}
	const TiXmlElement* controller = node->FirstChildElement("instance_controller");
	for(; controller; controller = controller->NextSiblingElement("instance_controller")) {
		vector<model::face> controller_faces;
		vector<model::bone> controller_bones;
		tie(controller_faces, controller_bones) = get_faces_and_bones_from_controller(resolve_shorthand_ptr(controller->Attribute("url")));
		bind_materials(controller, controller_faces);
		const TiXmlElement* skeleton = controller->FirstChildElement("skeleton");
		if(skeleton) {
			get_bones_from_skeleton(resolve_shorthand_ptr(skeleton->GetText()), controller_bones);
		}
		else
			continue;
		if(controller_bones.size() != 0 && bones.size() != 0)
			throw parsedae_error("Loading multiple skins from one file is not supported.");
		faces.insert(faces.end(), controller_faces.begin(), controller_faces.end());
		bones.insert(bones.end(), controller_bones.begin(), controller_bones.end());
	}
	const TiXmlElement* subnode = node->FirstChildElement("node");
	for(; subnode; subnode = subnode->NextSiblingElement("node")) {
		vector<model::face> subnode_faces;
		vector<model::bone> subnode_bones;
		tie(subnode_faces, subnode_bones) = get_faces_and_bones_from_node(subnode);
		faces.insert(faces.end(), subnode_faces.begin(), subnode_faces.end());
		bones.insert(bones.end(), subnode_bones.begin(), subnode_bones.end());
	}

	foreach(model::face& face, faces)
		face.transform = transform * face.transform;

	return make_pair(faces, bones);
}

void COLLADA::get_transform_from_node(const TiXmlElement* node, MatrixP3f& transform) const
{
	transform.loadIdentity();

	const TiXmlElement* transform_element = node->FirstChildElement();
	for(;transform_element; transform_element = transform_element->NextSiblingElement()) {
		if(transform_element->Value() == string("rotate")) {
			istringstream is(transform_element->GetText());
			Vector3f axis;
			is >> axis;
			float angle;
			is >> angle;
			transform.rotate3(M_PI/180*angle, axis);
		}
		if(transform_element->Value() == string("scale")) {
			istringstream is(transform_element->GetText());
			Vector3f coeffs;
			is >> coeffs;
			transform.scale(coeffs);
		}
		if(transform_element->Value() == string("translate")) {
			istringstream is(transform_element->GetText());
			Vector3f vector;
			is >> vector;
			transform.translate(vector);
		}
	}
}

void COLLADA::bind_materials(const TiXmlElement* instance, vector<model::face>& faces) const
{
	const TiXmlElement* material_instance =
		TiXmlHandle(const_cast<TiXmlElement*>(instance)).FirstChild("bind_material").FirstChild("technique_common").FirstChild("instance_material").ToElement();
	for(; material_instance; material_instance = material_instance->NextSiblingElement("instance_material")) {
		const_material_ptr material = get_material(resolve_shorthand_ptr(material_instance->Attribute("target")));
		foreach(model::face& face, faces) {
			if(face.material_name == string(material_instance->Attribute("symbol")))
				face.mat = material;
		}
	}
}

int COLLADA::get_bones_from_skeleton(const TiXmlElement* node, vector<model::bone>& bones) const
{
	const char* sid = node->Attribute("sid");
	for(int i = 0; i < bones.size(); i++) {
		model::bone& bone = bones[i];
		if(bone.name == string(sid)) {
			get_transform_from_node(node, bone.transform);
			const TiXmlElement* sub_node = node->FirstChildElement("node");
			for(; sub_node; sub_node = sub_node->NextSiblingElement("node")) {
				int child_num = get_bones_from_skeleton(sub_node, bones);
				if(child_num != -1)
					bone.children.push_back(child_num);
			}
			return i;
		}
	}
	return -1;
}

pair<vector<model::face>, vector<model::bone> > COLLADA::get_faces_and_bones_from_controller(const TiXmlElement* controller) const
{
	vector<model::face> faces;
	vector<model::bone> bones;
	multimap<int, model::vertex_ptr> vertices;

	const TiXmlElement* skin = controller->FirstChildElement("skin");
	if(skin) {
		vector<string> bone_ids;
		vector<MatrixP3f> inv_bind_matrices;
		const TiXmlElement* joints = skin->FirstChildElement("joints");
		const TiXmlElement* input = joints->FirstChildElement("input");
		for(;input; input = input->NextSiblingElement("input")) {
			if(input->Attribute("semantic") == string("JOINT")) {
				bone_ids = get_array<string>(resolve_shorthand_ptr(input->Attribute("source"))->FirstChildElement("IDREF_array"));
			}
			if(input->Attribute("semantic") == string("INV_BIND_MATRIX")) {
				inv_bind_matrices = get_array<MatrixP3f>(resolve_shorthand_ptr(input->Attribute("source"))->FirstChildElement("float_array"));
			}
		}
		for(int i = 0; i < bone_ids.size(); i++) {
			bones.push_back(model::bone());
			model::bone& bone = bones.back();
			bone.name = bone_ids[i];
			bone.inv_bind_matrix = inv_bind_matrices[i];
		}

		tie(faces, vertices) = get_faces_from_geometry(resolve_shorthand_ptr(skin->Attribute("source")));

		const TiXmlElement* bind_shape_matrix = skin->FirstChildElement("bind_shape_matrix");
		if(bind_shape_matrix) {
			istringstream is(bind_shape_matrix->GetText());
			MatrixP3f bind_shape_matrix;
			is >> bind_shape_matrix;
			foreach(model::face& face, faces)
				face.transform = bind_shape_matrix;
		}

		const TiXmlElement* vertex_weights = skin->FirstChildElement("vertex_weights");
		input = vertex_weights->FirstChildElement("input");
		int max_offset=0, offset, vertex_count;
		vertex_weights->QueryIntAttribute("count", &vertex_count);
		int joint_offset=0, weight_offset=0;
		vector<int> vcounts = get_array<int>(vertex_weights->FirstChildElement("vcount"));
		vector<int> v_indices = get_array<int>(vertex_weights->FirstChildElement("v"));
		vector<float> weights;
		for(;input; input = input->NextSiblingElement("input")) {
			input->QueryIntAttribute("offset", &offset);
			if(offset > max_offset) max_offset = offset;
			if(input->Attribute("semantic") == string("JOINT")) {
				joint_offset = offset;
			}
			if(input->Attribute("semantic") == string("WEIGHT")) {
				weight_offset = offset;
				weights = get_array<float>(resolve_shorthand_ptr(input->Attribute("source"))->FirstChildElement("float_array"));
			}
		}

		for(int i = 0, v = 0; i < vertex_count; i++) {
			int vcount = vcounts[i];
			vector<pair<int, float> > influences;
			for(int j = 0; j < vcount; j++) {
				influences.push_back(make_pair(v_indices[v + j*(max_offset+1) + joint_offset], weights[v_indices[v + j*(max_offset+1) + weight_offset]]));
			}
			multimap<int, model::vertex_ptr>::iterator vertex, v_end;
			tie(vertex, v_end) = vertices.equal_range(i);
			for(; vertex != v_end; ++vertex) {
				vertex->second->influences = influences;
			}
			v += (max_offset+1)*vcount;
		}
	}
	return make_pair(faces, bones);
}

pair<vector<model::face>, multimap<int, model::vertex_ptr> > COLLADA::get_faces_from_geometry(const TiXmlElement* geometry) const
{
	vector<model::face> faces;
	multimap<int, model::vertex_ptr> vertices;
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
						vertices.insert(make_pair(primitive_indices[i + positions_offset], vertex));
						vertex->point[0] = positions[primitive_indices[i + positions_offset]*3];
						vertex->point[1] = positions[primitive_indices[i + positions_offset]*3 + 1];
						vertex->point[2] = positions[primitive_indices[i + positions_offset]*3 + 2];
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
	return make_pair(faces, vertices);
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
	mat->set_shininess(12.5);
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
					mat->set_texture(basename((char*)image->GetText()));
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
				if(color->Value() == string("shininess")) {
					const TiXmlElement* shininess_value = color->FirstChildElement("float");
					if(shininess_value) {
						float shininess;
						istringstream is(shininess_value->GetText());
						is >> shininess;
						mat->set_shininess(shininess);
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
	vector<model::face> faces;
	vector<model::bone> bones;
	std::cerr << "Parsing COLLADA...\n";
	COLLADA collada(i1);
	tie(faces, bones) = collada.get_faces_and_bones();
	foreach(model::face& face, faces) {
		face.transform.scale(ScaleFactor);
		scale_translation_vector(face.transform);
	}
	foreach(model::bone& bone, bones) {
		bone.transform.scale(ScaleFactor);
		scale_translation_vector(bone.transform);
		bone.inv_bind_matrix.scale(ScaleFactor);
		scale_translation_vector(bone.inv_bind_matrix);
	}
	return(model_ptr(new model(faces, bones)));
}

}
