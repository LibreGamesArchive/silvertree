#include "boost/array.hpp"
#include "foreach.hpp"
#include "model.hpp"
#include "parsedae.hpp"
#include "xml/xml.h"

#include <iostream>
#include <map>

namespace graphics
{

namespace {
const GLfloat ScaleFactor = 0.009;
}

void parse_mesh(XML_PARSER* parser, std::vector<model::face>& faces)
{
	std::vector<GLfloat> position, normal, uv;
	xml_skip_attributes(parser);
	XML_TOKEN token;
	while(xml_get_token(parser, &token) &&
	      token.type != XML_TOKEN_END_ELEMENT) {
		if(token.type != XML_TOKEN_BEGIN_ELEMENT) {
			continue;
		}

		if(XML_TOKEN_EQUALS(token, "source")) {
			std::cerr << "source...\n";
			XML_TOKEN name, value, token;
			std::vector<GLfloat>* v = NULL;
			while(xml_get_attr(parser, &name, &value)) {
				std::cerr << "name '" << std::string(name.str,name.str+name.length) << "' -> '" << std::string(value.str,value.str+value.length) << "'\n";
				if(XML_TOKEN_EQUALS(name, "id")) {
					const std::string val(value.str,value.str+value.length);
					if(strcasestr(val.c_str(),"position")) {
						std::cerr << "position...\n";
						v = &position;
					} else if(strcasestr(val.c_str(),"normal")) {
						std::cerr << "normal...\n";
						v = &normal;
					} else if(strcasestr(val.c_str(),"uv")) {
						v = &uv;
					} else {
						assert(false);
					}
				}
			}

			while(xml_get_child(parser, &token)) {
				if(XML_TOKEN_EQUALS(token, "float_array")) {
					XML_TOKEN text;
					while(xml_get_token(parser, &text) &&
					      text.type != XML_TOKEN_END_ELEMENT) {
						if(text.type == XML_TOKEN_BEGIN_ELEMENT) {
							xml_skip_element(parser);
						} else if(text.type == XML_TOKEN_TEXT) {
							const char* pos = text.str;
							char* endptr = NULL;
							v->push_back(strtod(pos,&endptr));
							char* ptr = NULL;
							while(ptr != endptr) {
								ptr = endptr;
								v->push_back(strtod(ptr,&endptr));
							}

							v->pop_back();
						}
					}
				} else {
					xml_skip_element(parser);
				}
			}
		} else if(XML_TOKEN_EQUALS(token, "triangles")) {
			material_ptr mat;
			XML_TOKEN name, value, token;
			while(xml_get_attr(parser, &name, &value)) {
				if(XML_TOKEN_EQUALS(name, "material")) {
					const GLfloat material_data[] = {1.0,1.0,1.0,1.0};
					mat.reset(new material);
					mat->set_texture(std::string(value.str,value.str+value.length) + ".jpg");
					mat->set_ambient(material_data);
					mat->set_diffuse(material_data);
					mat->set_specular(material_data);
				}
			}

			bool found_vertex = false, found_normal = false, found_uv = false;

			while(xml_get_token(parser, &token) &&
			      token.type != XML_TOKEN_END_ELEMENT) {
				if(token.type == XML_TOKEN_BEGIN_ELEMENT) {
					if(XML_TOKEN_EQUALS(token, "input")) {
						XML_TOKEN name, value;
						while(xml_get_attr(parser, &name, &value)) {
							if(XML_TOKEN_EQUALS(name, "semantic")) {
								if(XML_TOKEN_EQUALS(value, "VERTEX")) {
									found_vertex = true;
								} else if(XML_TOKEN_EQUALS(value, "NORMAL")) {
									found_normal = true;
								} else if(XML_TOKEN_EQUALS(value, "TEXCOORD")) {
									found_uv = true;
								}
							}
						}
						xml_skip_element(parser);
					} else if(XML_TOKEN_EQUALS(token, "p") && found_vertex &&
						      found_normal && found_uv) {
						XML_TOKEN text;
						while(xml_get_token(parser, &text) &&
						      text.type != XML_TOKEN_END_ELEMENT) {
							if(text.type == XML_TOKEN_BEGIN_ELEMENT) {
								xml_skip_element(parser);
								continue;
							}

							if(text.type == XML_TOKEN_TEXT) {
								const char* pos = text.str;
								char* endptr = NULL;

								std::vector<int> items;
								items.push_back(strtol(pos,&endptr,10));
								char* ptr = NULL;
								while(ptr != endptr) {
									ptr = endptr;
									items.push_back(strtol(ptr,&endptr,10));
								}

								items.pop_back();

								typedef boost::array<int,3> Key;
								std::map<Key,model::vertex_ptr> map;

								std::vector<model::vertex_ptr> vertices;
								for(int n = 0; n <= items.size()-3; n += 3) {
									Key k;
									for(int m = 0; m != k.size(); ++m) {
										k[m] = items[n+m];
									}

									std::map<Key,model::vertex_ptr>::const_iterator i = map.find(k);
									if(i != map.end()) {
										vertices.push_back(i->second);
										continue;
									}

									model::vertex_ptr v(new model::vertex());
									v->point[0] = position[items[n]*3]*ScaleFactor;
									v->point[1] = position[items[n]*3+1]*ScaleFactor;
									v->point[2] = position[items[n]*3+2]*ScaleFactor;
									v->normal[0] = normal[items[n+1]*3];
									v->normal[1] = normal[items[n+1]*3+1];
									v->normal[2] = normal[items[n+1]*3+2];
									v->uvmap[0] = uv[items[n+2]*2];
									v->uvmap[1] = -uv[items[n+2]*2+1];
									v->uvmap_valid = true;
									vertices.push_back(v);
									map[k] = v;
								}

								for(int n = 0; n <= vertices.size()-3; n += 3) {
									faces.push_back(model::face());
									model::face& f = faces.back();
									f.mat = mat;
									f.vertices.push_back(vertices[n]);
									f.vertices.push_back(vertices[n+1]);
									f.vertices.push_back(vertices[n+2]);
								}
							}
						}
					} else {
						xml_skip_element(parser);
					}
				}
			}
		} else {
			xml_skip_element(parser);
		}
	}

	std::cerr << "faces: " << faces.size() << "\n";
}

model_ptr parsedae(const char* i1, const char* i2)
{
	XML_PARSER parser;
	xml_init_parser(&parser, i1);

	std::vector<const_material_ptr> materials;
	std::vector<model::face> faces;
	std::vector<model::bone> bones;

	XML_TOKEN token;
	while(xml_get_token(&parser, &token)) {
		if(token.type == XML_TOKEN_BEGIN_ELEMENT &&
		   XML_TOKEN_EQUALS(token,"mesh")) {
			parse_mesh(&parser, faces);
		}
	}

	return model_ptr(new graphics::model(faces, bones));
}

}
