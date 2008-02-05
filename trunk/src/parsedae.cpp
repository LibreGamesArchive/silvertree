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

const TiXmlElement* resolve_shorthand_ptr(string id, map<string,const TiXmlElement*>& named_elements)
{
	if(id[0] != '#') return NULL;
	id.erase(id.begin());
	return named_elements[id];
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
	TiXmlDocument doc;
	doc.Parse(i1);
	if(doc.Error()) {
		std::cerr << "Parse error: " << doc.ErrorDesc() << std::endl;
	}

	TiXmlHandle root(doc.RootElement());
	if(root.ToElement()->Value() != string("COLLADA")) {
		std::cerr << "Not a COLLADA document. Bailing out.\n";
		return (model_ptr(new model(vector<model::face>())));
	}

	map<string,const TiXmlElement*> named_elements;
	struct name_collector_t : public TiXmlVisitor
	{
		map<string,const TiXmlElement*>& named_elements_;
		name_collector_t(map<string,const TiXmlElement*>& named_elements) 
			: named_elements_(named_elements) {}
		bool VisitEnter(const TiXmlElement& element, const TiXmlAttribute* attribute)
		{
			const char* id = element.Attribute("id");
			if(id) {
				named_elements_[id] = &element;
			}
			return true;
		}
	} name_collector(named_elements);
	root.ToElement()->Accept(&name_collector);

	std::vector<model::face> faces;
	TiXmlElement* geometry = root.FirstChildElement("library_geometries").FirstChild("geometry").ToElement();
	for(;geometry; geometry = geometry->NextSiblingElement("geometry")) {
		TiXmlElement* mesh = geometry->FirstChildElement("mesh");
		if(mesh) {
			TiXmlElement* source = mesh->FirstChildElement("source");
			map<string, vector<GLfloat> > sources;
			for(;source; source = source->NextSiblingElement("source")) {
				TiXmlElement* float_array = source->FirstChildElement("float_array");
				if(!float_array) continue;
				string source_text(float_array->GetText());
				istringstream is(source_text);
				vector<GLfloat> source_floats;
				while(is.good()) {
					GLfloat num;
					is >> num;
					source_floats.push_back(num);
				}
				sources.insert(std::make_pair(string(source->Attribute("id")), source_floats));
			}

			TiXmlElement* triangles = mesh->FirstChildElement("triangles");
			for(;triangles; triangles = triangles->NextSiblingElement("triangles")) {

				vector<GLfloat>* position_source = NULL;
				int position_offset;
				vector<GLfloat>* normal_source = NULL;
				int normal_offset;
				vector<GLfloat>* uvmap_source = NULL;
				int uvmap_offset;

				TiXmlElement* input = triangles->FirstChildElement("input");
				for(;input; input = input->NextSiblingElement("input")) {
					if(input->Attribute("semantic") == string("VERTEX")) {
						input->QueryIntAttribute("offset", &position_offset);
						const TiXmlElement* vertices;
						vertices = resolve_shorthand_ptr(input->Attribute("source"), named_elements);
						string pos_source_ptr = vertices->FirstChildElement("input")->Attribute("source");
						pos_source_ptr.erase(pos_source_ptr.begin());
						position_source = &sources[pos_source_ptr];
					}
					if(input->Attribute("semantic") == string("NORMAL")) {
						input->QueryIntAttribute("offset", &normal_offset);
						string normal_source_ptr(input->Attribute("source"));
						normal_source_ptr.erase(normal_source_ptr.begin());
						normal_source = &sources[normal_source_ptr];
					}
					if(input->Attribute("semantic") == string("TEXCOORD")) {
						input->QueryIntAttribute("offset", &uvmap_offset);
						string uvmap_source_ptr(input->Attribute("source"));
						uvmap_source_ptr.erase(uvmap_source_ptr.begin());
						uvmap_source = &sources[uvmap_source_ptr];
					}
				}

				const TiXmlElement* p = triangles->FirstChildElement("p");
				vector<int> primitives;
				string primitives_text(p->GetText());
				istringstream is(primitives_text);
				while(is.good()) {
					int num;
					is >> num;
					primitives.push_back(num);
				}
				faces.push_back(model::face());
				model::face& face = faces.back();
				int count;
				triangles->QueryIntAttribute("count", &count);

				for(int i = 0; i < count * 3; i++) {
					face.vertices.push_back(model::vertex_ptr(new model::vertex));
					model::vertex_ptr vertex = face.vertices.back();
					vertex->point[0] = position_source->at(primitives[i*3 + position_offset]*3) * ScaleFactor;
					vertex->point[1] = position_source->at(primitives[i*3 + position_offset]*3 + 1) * ScaleFactor;
					vertex->point[2] = position_source->at(primitives[i*3 + position_offset]*3 + 2) * ScaleFactor;
					vertex->normal[0] = -normal_source->at(primitives[i*3 + normal_offset]*3);
					vertex->normal[1] = -normal_source->at(primitives[i*3 + normal_offset]*3 + 1);
					vertex->normal[2] = -normal_source->at(primitives[i*3 + normal_offset]*3 + 2);
					vertex->uvmap[0] = uvmap_source->at(primitives[i*3 + uvmap_offset]*2);
					vertex->uvmap[1] = -uvmap_source->at(primitives[i*3 + uvmap_offset]*2 + 1);
					vertex->uvmap_valid = true;
				}

				material_ptr mat(new material);
				GLfloat material_data[] = {1.0,1.0,1.0,1.0};
				mat->set_ambient(material_data);
				mat->set_diffuse(material_data);
				mat->set_specular(material_data);
				const char* material_name = triangles->Attribute("material");
				const TiXmlElement* material_element = named_elements[material_name];
				if(!material_element)
					material_element = named_elements[string(material_name) + "ID"];
				if(material_element) {
					bool have_texture = false;
					const char* effect_url = material_element->FirstChildElement("instance_effect")->Attribute("url");
					const TiXmlElement* effect_element = resolve_shorthand_ptr(effect_url, named_elements);
					const TiXmlElement* newparam = effect_element->FirstChildElement("profile_COMMON")->FirstChildElement("newparam");
					const TiXmlElement* technique = effect_element->FirstChildElement("profile_COMMON")->FirstChildElement("technique");
					for(;newparam;newparam = newparam->NextSiblingElement("newparam")) {
						const TiXmlElement* surface = newparam->FirstChildElement("surface");
						if(surface && surface->Attribute("type") == string("2D")) {
							const TiXmlElement* image = surface->FirstChildElement("init_from");
							if(image) {
								image = named_elements[image->GetText()];
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
				} else {
					mat->set_texture("");
				}
				face.mat = mat;
			}
		}
	}

	return(model_ptr(new model(faces)));
}

}
