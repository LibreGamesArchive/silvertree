
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
#include "material.hpp"
#include "model.hpp"
#include "parse3ds.hpp"

#include <algorithm>
#include <map>
#include <stdint.h>

#include <iostream>

namespace {

template<typename T>
void fix_endian(T& t)
{
#ifdef WORDS_BIGENDIAN
	char* i1 = reinterpret_cast<char*>(&t);
	char* i2 = i1 + sizeof(T);
	std::reverse(i1,i2);
#endif
}

template<typename T>
T read_raw(const char*& i1, const char* i2) {
	if(i2 - i1 < sizeof(T)) {
		std::cerr << "end of 3ds input when reading data\n";
		throw graphics::parse3ds_error();
	}

	T res;
	char* dst = reinterpret_cast<char*>(&res);
	std::copy(i1,i1+sizeof(T),dst);
	i1 += sizeof(T);
	return res;
}

template<typename T>
T read_integer(const char*& i1, const char* i2) {
	T res = read_raw<T>(i1,i2);
	fix_endian(res);
	return res;
}

std::string read_string(const char*& i1, const char* i2) {
	const char* end_str = std::find(i1,i2,0);
	if(end_str == i2) {
		std::cerr << "could not find string terminator\n";
		throw graphics::parse3ds_error();
	}

	const std::string res(i1,end_str);
	i1 = end_str+1;
	return res;
}

struct chunk
{
	uint16_t id;
	const char* beg;
	const char* end;
};

chunk read_chunk(const char*& i1, const char* i2) {
	chunk res;
	res.id = read_integer<uint16_t>(i1,i2);
	const uint32_t size = read_integer<uint32_t>(i1,i2) - 6;
	if(size > i2 - i1) {
		std::cerr << "chunk size of " << size
				  << " read, but only " << (i2-i1)
				  << " bytes in input\n";
		throw graphics::parse3ds_error();
	}
	
	res.beg = i1;
	i1 = res.end = i1 + size;
	return res;
}

chunk read_child_chunk(chunk& c) {
	return read_chunk(c.beg,c.end);
}

std::vector<graphics::model::face> parse_mesh(chunk& c)
{
	std::vector<graphics::model::vertex_ptr> vertices;
	std::vector<graphics::model::face> res;

	const GLfloat ScaleFactor = 0.005;

	while(c.beg != c.end) {
		chunk child = read_child_chunk(c);
		switch(child.id) {
		case 0x4110: { //vertices
			const GLfloat* lowest = NULL;
			uint16_t num = read_integer<uint16_t>(child.beg,child.end);

			boost::array<const GLfloat*,3> highs, lows;
			std::fill(highs.begin(),highs.end(),(GLfloat*)NULL);
			std::fill(lows.begin(),lows.end(),(GLfloat*)NULL);

			for(uint16_t n = 0; n != num; ++n) {
				graphics::model::vertex_ptr v(
				             new graphics::model::vertex);
				for(unsigned int m = 0; m != v->point.size(); ++m) {
					v->point[m] = ScaleFactor*
					  read_raw<GLfloat>(child.beg,child.end);

					if(lows[m] == NULL || v->point[m] < *lows[m]) {
						lows[m] = &v->point[m];
					}

					if(highs[m] == NULL || v->point[m] > *highs[m]) {
						highs[m] = &v->point[m];
					}
				}


				if(lowest == NULL || v->point[2] < *lowest) {
					lowest = &v->point[2];
				}
				vertices.push_back(v);
			}

			if(lowest != NULL && *lowest < 0.0) {
				const GLfloat low = *lowest;
				foreach(graphics::model::vertex_ptr& v, vertices) {
					v->point[2] -= low;
				}
			}

			if(lows[0]) {
				std::cerr << "dimensions: " << *lows[0] << "-" << *highs[0] << "," << *lows[1] << "-" << *highs[1] << "," << *lows[2] << "-" << *highs[2] << "\n";
			}

			std::cerr << "read " << num << " vertices\n";
		}
		break;
		case 0x4120: { //faces
			uint16_t num = read_integer<uint16_t>(child.beg,child.end);
			for(uint16_t n = 0; n != num; ++n) {
				graphics::model::face f;
				f.vertices.resize(3);
				for(unsigned int m = 0; m != f.vertices.size(); ++m) {
					const uint16_t index = read_integer<uint16_t>(
					                     child.beg,child.end);
					if(index >= vertices.size()) {
						std::cerr << "illegal vertex index "
						          << index << "/" << vertices.size()
								  << "\n";
						throw graphics::parse3ds_error();
					}

					f.vertices[m] = vertices[index];
				}

				/*uint16_t face_flag = read_integer<uint16_t>(
				                      child.beg,child.end);
				face_flag; //currently ignored
				*/
				res.push_back(f);
			}

			std::cerr << "loaded " << res.size() << " faces\n";

			while(child.beg != child.end) {
				chunk face_info = read_child_chunk(child);
				switch(face_info.id) {
				case 0x4130: {
					const std::string name = read_string(
					           face_info.beg,face_info.end);
					uint16_t num = read_integer<uint16_t>(
					           face_info.beg,face_info.end);
					for(uint16_t n = 0; n != num; ++n) {
						const uint16_t index =
						           read_integer<uint16_t>(
						       face_info.beg,face_info.end);
						if(index >= res.size()) {
							std::cerr << "illegal face index "
							          << index << "/"
									  << res.size() << "\n";
							throw graphics::parse3ds_error();
						}

						res[index].material_name = name;
					}
				}
				break;
				}
			}
		}
		break;
		case 0x4140: {
			uint16_t num = read_integer<uint16_t>(child.beg,child.end);
			for(uint16_t n = 0; n != num; ++n) {
				if(n >= vertices.size()) {
					break;
				}

				vertices[n]->uvmap[0] = read_raw<GLfloat>(
				                             child.beg,child.end);
				vertices[n]->uvmap[1] = read_raw<GLfloat>(
				                             child.beg,child.end);
				vertices[n]->uvmap_valid = true;

			}
		}
		break;
		case 0x4160: { //local co-ordinate system
			for(int i = 0; i != 4; ++i) {
				std::cerr << "local co-ordinates " << i << ": ";
				boost::array<GLfloat,3> v;
				for(int j = 0; j != 3; ++j) {
					v[j] = read_raw<GLfloat>(child.beg,child.end);
					std::cerr << v[j] << ",";
				}
				std::cerr << "\n";
			}
		}
		break;
		}
	}

	return res;
}

std::vector<graphics::model::face> parse_object(chunk& c)
{
	std::cerr << "parse object...\n";
	const std::string name = read_string(c.beg,c.end);

	std::vector<graphics::model::face> res;

	while(c.beg != c.end) {
		chunk child = read_child_chunk(c);
		switch(child.id) {
			case 0x4100:
				std::vector<graphics::model::face> faces
				    = parse_mesh(child);
				std::copy(faces.begin(),faces.end(),
				          std::back_inserter(res));
			break;
		}
	}

	return res;
}

std::string parse_texture_map(chunk& c)
{
	std::string res;
	while(c.beg != c.end) {
		chunk child = read_chunk(c.beg,c.end);
		switch(child.id) {
		case 0xA300: { //mapping filename
			res = read_string(child.beg,child.end);
		}
		break;
		}
	}

	return res;
}

boost::array<GLfloat,3> parse_color(chunk& c)
{
	boost::array<GLfloat,3> res;
	while(c.beg != c.end) {
		chunk child = read_chunk(c.beg,c.end);
		switch(child.id) {
		case 0x0010: { //float format
			for(int i = 0; i != 3; ++i) {
				res[i] = read_raw<GLfloat>(child.beg,child.end);
			}
			break;
		}
		case 0x0011: { //byte format
			for(int i = 0; i != 3; ++i) {
				const unsigned char c = read_raw<unsigned char>(
				                               child.beg,child.end);

				res[i] = static_cast<GLfloat>(c)/255.0;
			}

			break;
		}
		}
	}

	return res;
}

void parse_material(
   chunk& c,std::map<std::string,graphics::material_ptr>& materials)
{
	std::cerr << "parsing material...\n";
	std::string name;
	graphics::material_ptr mat(new graphics::material);
	while(c.beg != c.end) {
		chunk child = read_chunk(c.beg,c.end);
		switch(child.id) {
		case 0xA000: { //material name
			name = read_string(child.beg,child.end);
			std::cerr << "material name '" << name << "'\n";
			break;
		}
		case 0xA010: { //ambient color
			boost::array<GLfloat,3> color = parse_color(child);
			std::cerr << "ambient: " << color[0] << "," << color[1] << "," << color[2] << "\n";
			mat->set_ambient(color.data());
			break;
		}
		case 0xA020: { //diffuse color
			boost::array<GLfloat,3> color = parse_color(child);
			std::cerr << "diffuse: " << color[0] << "," << color[1] << "," << color[2] << "\n";
			mat->set_diffuse(color.data());
			break;
		}
		case 0xA030: { //specular color
			boost::array<GLfloat,3> color = parse_color(child);
			std::cerr << "specular: " << color[0] << "," << color[1] << "," << color[2] << "\n";
			mat->set_specular(color.data());
			break;
		}
		case 0xA200: { //texture map 1
			std::cerr << "texture map...\n";
			const std::string tex = parse_texture_map(child);
			mat->set_texture(tex);
			break;
		}
		default: {
			break;
		}
		}
	}

	materials.insert(
	   std::pair<std::string,graphics::material_ptr>(name, mat));

	std::cerr << "done parsing material...\n";
}

std::vector<graphics::model::face> parse_editor(chunk& c)
{
	std::cerr << "parse editor...\n";
	std::map<std::string,graphics::material_ptr> materials;
	std::vector<graphics::model::face> res;
	while(c.beg != c.end) {
		chunk child = read_child_chunk(c);
		switch(child.id) {
		case 0x4000: {
			std::vector<graphics::model::face> faces = parse_object(child);
			std::copy(faces.begin(),faces.end(),
			          std::back_inserter(res));
			std::cerr << "parsed object: " << res.size() << "\n";
			break;
		}
		case 0xAFFF:
			parse_material(child,materials);
			break;
		}
	}

	foreach(graphics::model::face& f,res) {
		f.mat = materials[f.material_name];
		if(!f.mat) {
			std::cerr << "no material found!\n";
		}
	}

	return res;
}

void parse_mesh_keyframes(chunk& c)
{
	while(c.beg != c.end) {
		chunk child = read_child_chunk(c);
		switch(child.id) {
		case 0xB010: { //object name, parameters, father
			const std::string name = read_string(child.beg,child.end);
			/*const uint16_t flag1 =*/ read_integer<uint16_t>(
			                                child.beg,child.end);
			/*const uint16_t flag2 =*/ read_integer<uint16_t>(
			                                child.beg,child.end);
			const uint16_t parent = read_integer<uint16_t>(
			                                child.beg,child.end);

			std::cerr << "key frame '" << name << "' parented by " << parent << "\n";
			break;
		}
		case 0xB013: { //object pivot point
			std::cerr << "pivot ";
			for(int n = 0; n != 3; ++n) {
				const GLfloat point = read_raw<GLfloat>(
				                            child.beg, child.end);
				std::cerr << point << ",";
			}
			std::cerr << "\n";
			break;
		}
		}
	}
}

void parse_keyframer(chunk& c)
{
	while(c.beg != c.end) {
		chunk child = read_child_chunk(c);
		switch(child.id) {
		case 0xB002: { //mesh information -- the only one we care about
			parse_mesh_keyframes(child);
			break;
		}
		case 0xB008: {
			const uint16_t begin_frames = read_integer<uint16_t>(
			                       child.beg,child.end);
			const uint16_t end_frames = read_integer<uint16_t>(
			                       child.beg,child.end);
			std::cerr << "frames: " << begin_frames << " .. "
			          << end_frames << "\n";
			break;
		}
		}
	}
}
		
}

namespace graphics
{

model_ptr parse3ds(const char* i1, const char* i2)
{
	std::vector<graphics::model::face> faces;
	chunk main_chunk = read_chunk(i1,i2);
	std::cerr << "parsing main chunk...\n";

	if(main_chunk.id != 0x4D4D) {
		std::cerr << "main chunk does not have correct ID\n";
		throw parse3ds_error();
	}

	while(main_chunk.beg != main_chunk.end) {
		chunk c = read_child_chunk(main_chunk);
		switch(c.id) {
		case 0x3D3D: { //object block
			std::vector<graphics::model::face> new_faces =
			                           parse_editor(c);
			std::copy(new_faces.begin(),new_faces.end(),
			          std::back_inserter(faces));
			break;
		}

		case 0xB000: { //keyframer block
			parse_keyframer(c);
			break;
		}
		}
	}

	std::cerr << "returning model with " << faces.size() << " faces\n";

	model_ptr res(new model(faces));

	std::cerr << "made model...\n";
	return res;
}
		
}

#if 0
int main()
{
	const std::string data = sys::read_file("chr-tansan.3ds");
	if(data.empty()) {
		std::cerr << "could not read 3ds file\n";
		return 0;
	}
		
	const char* d = data.c_str();
	try {
		graphics::parse3ds(d,d+data.size());
	} catch(graphics::parse3ds_error&) {
		std::cerr << "error parsing 3ds...\n";
	}
}

#endif
