
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <gl.h>
#include <glu.h>
#include <SDL.h>

#include <cmath>
#include <iostream>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../base_terrain.hpp"
#include "../camera.hpp"
#include "../filesystem.hpp"
#include "../font.hpp"
#include "../foreach.hpp"
#include "../gamemap.hpp"
#include "../raster.hpp"
#include "../terrain_feature.hpp"
#include "../tile.hpp"
#include "../wml_parser.hpp"

struct undo_info {
	std::vector<hex::tile> tiles;
	std::set<hex::location> locs;
};

std::stack<undo_info> undo_stack;

namespace {
void clone_hex(hex::gamemap& map, const hex::tile& src, const hex::location& loc)
{
	const hex::tile& dst = map.get_tile(loc);
	map.adjust_height(loc,src.height() - dst.height());
	map.set_terrain(loc,src.terrain()->id());
	hex::const_terrain_feature_ptr f = src.feature();
	std::string feature_id;
	if(f) {
		feature_id = f->id();
	}

	map.set_feature(loc,feature_id);
}
}

int main(int argc, char** argv)
{
	if(argc != 2) {
		std::cerr << "usage: " << argv[0] << " <mapname>\n";
		return 0;
	}

	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	if(SDL_SetVideoMode(1024,768,0,SDL_OPENGL) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}

	SDL_EnableUNICODE(1);

	graphics::font::manager font_manager;

	wml::node_ptr game_cfg;

	{
	const int fd = open("game.cfg",O_RDONLY);
	if(fd < 0) {
		std::cerr << "could not open map\n";
		return -1;
	}

	struct stat fileinfo;
	fstat(fd,&fileinfo);

	std::vector<char> filebuf(fileinfo.st_size);
	read(fd,&filebuf[0],fileinfo.st_size);
	std::string doc(filebuf.begin(),filebuf.end());
	try {
		game_cfg = wml::parse_wml(doc);
	} catch(...) {
		std::cerr << "error parsing WML...\n";
		return -1;
	}
	close(fd);
	}

	wml::node::const_all_child_iterator cfg1 = game_cfg->begin_children();
	wml::node::const_all_child_iterator cfg2 = game_cfg->end_children();
	while(cfg1 != cfg2) {
		if((*cfg1)->name() == "terrain") {
			hex::base_terrain::add_terrain(*cfg1);
		} else if((*cfg1)->name() == "terrain_feature") {
			hex::terrain_feature::add_terrain(*cfg1);
		}

		++cfg1;
	}

	std::string mapdata;

	{
	const int fd = open(argv[1],O_RDONLY);
	if(fd < 0) {
		std::cerr << "could not open map\n";
		return -1;
	}

	struct stat fileinfo;
	fstat(fd,&fileinfo);

	std::vector<char> filebuf(fileinfo.st_size);
	read(fd,&filebuf[0],fileinfo.st_size);
	mapdata.assign(filebuf.begin(),filebuf.end());
	close(fd);
	}

	GLfloat material_specular[] = {1.0,1.0,1.0,1.0};
	GLfloat material_shininess[] = {1000.0};
	GLfloat intensity = 1.0;
	GLfloat ambient_light[] = {intensity,intensity,intensity,1.0};
	GLfloat diffuse_light[] = {1.0,1.0,1.0,1.0};

	GLfloat dim_ambient[] = {0.5,0.5,0.5,1.0};
	GLfloat dim_diffuse[] = {0.5,0.5,0.5,1.0};

	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,material_specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,material_specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,material_specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,material_shininess);

	glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambient_light);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse_light);
	glLightfv(GL_LIGHT2,GL_AMBIENT,dim_ambient);
	glLightfv(GL_LIGHT2,GL_DIFFUSE,dim_diffuse);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.05);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	hex::gamemap map(mapdata);
	hex::camera camera(map);
	camera.allow_keyboard_panning();
	bool done = false;
	bool show_grid = true;
	int radius = 0;
	std::vector<hex::location> locs;
	bool pick_mode = false;
	hex::location picked_loc;
	std::string current_terrain;
	std::string current_feature;
	bool new_mutation = true;
	while(!done) {
		glEnable(GL_LIGHT0);
		hex::location selected;
		GLfloat xscroll = -camera.get_pan_x();
		GLfloat yscroll = -camera.get_pan_y();
		const hex::tile* center = map.closest_tile(&xscroll,&yscroll);
		if(center) {
			camera.prepare_selection();
			locs.clear();
			GLuint select_name = 0;
			for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
				for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
					hex::location loc(x,y);
					if(map.is_loc_on_map(loc)) {
						const hex::tile& t = map.get_tile(loc);
						locs.push_back(loc);
						glLoadName(select_name++);
						t.draw();
					}
				}
			}

			select_name = camera.finish_selection();
			if(select_name < locs.size()) {
				selected = locs[select_name];
			}

			camera.prepare_frame();

			for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
				for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
					hex::location loc(x,y);
					if(map.is_loc_on_map(loc)) {
						const hex::tile& t = map.get_tile(loc);
						t.draw();
						t.draw_model();
					}
				}
			}

			for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
				for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
					hex::location loc(x,y);
					if(map.is_loc_on_map(loc)) {
						const hex::tile& t = map.get_tile(loc);
						t.draw_cliffs();
					}
				}
			}

			if(show_grid) {
				glDisable(GL_LIGHTING);
				for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
					for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
						hex::location loc(x,y);
						if(map.is_loc_on_map(loc)) {
							const hex::tile& t = map.get_tile(loc);
							t.draw_grid();
						}
					}
				}
				glEnable(GL_LIGHTING);
			}
		} else {
			map.draw();
			map.draw_grid();
		}

		if(map.is_loc_on_map(selected)) {
			for(int n = 0; n <= radius; ++n) {
				std::vector<hex::location> locs;
				hex::get_tile_ring(selected, n, locs);
				foreach(const hex::location& loc, locs) {
					if(map.is_loc_on_map(loc)) {
						const hex::tile& t = map.get_tile(loc);
						t.draw_highlight();
					}
				}
			}
		}

		{
		const SDL_Color color = {0xFF,0xFF,0x0,0};
		std::ostringstream s;
		s << "(" << selected.x() << "," << selected.y();
		if(map.is_loc_on_map(selected)) {
			const hex::tile& t = map.get_tile(selected);
			s << "," << t.height();
		}
		s << ")";
		graphics::texture text = graphics::font::render_text(s.str(),20,color);
		graphics::prepare_raster();
		graphics::blit_texture(text,20,20);
		if(pick_mode) {
			std::string pick_text;
			if(map.is_loc_on_map(picked_loc)) {
				const hex::tile& t = map.get_tile(picked_loc);
				std::ostringstream s;
				s << "pick: " << t.terrain()->name() << " " << t.height();
				pick_text = s.str();
			} else {
				pick_text = "pick mode";
			}

			text = graphics::font::render_text(pick_text,20,color);
			graphics::blit_texture(text,20,50);
		} else if(current_feature.empty() == false) {
			hex::const_terrain_feature_ptr t = hex::terrain_feature::get(current_feature);
			if(t) {
				text = graphics::font::render_text(t->name(),20,color);
				graphics::blit_texture(text,20,50);
			}
		} else {
			hex::const_base_terrain_ptr t = hex::base_terrain::get(current_terrain);
			if(t) {
				text = graphics::font::render_text(t->name(),20,color);
				graphics::blit_texture(text,20,50);
			}
		}
		}

		SDL_GL_SwapBuffers();
		
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					done = true;
					break;
				case SDL_KEYDOWN: {
					const SDL_KeyboardEvent& e = event.key;
					const bool ctrl = (e.keysym.mod&KMOD_CTRL) != 0;
					const char c = char(e.keysym.unicode);
					std::string s(1,c);
					if(hex::base_terrain::get(s)) {
						std::cerr << "set terrain '" << s << "'\n";
						current_terrain = s;
						current_feature.clear();
						pick_mode = false;
					} else if(hex::terrain_feature::get(s)) {
						std::cerr << "set feature '" << s << "'\n";
						current_feature = s;
						current_terrain.clear();
						pick_mode = false;
					}

					if(e.keysym.sym == SDLK_SPACE) {
						current_terrain = "";
						pick_mode = false;
					}

					if(ctrl && e.keysym.sym == SDLK_s) {
						sys::write_file(argv[1], map.write());
						std::cerr << "saved map\n";
					}

					if(ctrl && e.keysym.sym == SDLK_p) {
						pick_mode = true;
					}

					if(ctrl && e.keysym.sym == SDLK_z && !undo_stack.empty()) {
						std::cerr << "undoing...\n";
						const undo_info& undo = undo_stack.top();
						foreach(const hex::tile& t, undo.tiles) {
							clone_hex(map,t,t.loc());
						}

						undo_stack.pop();
					}

					break;
				}
				default:
					break;
			}
		}

		const Uint8* keys = SDL_GetKeyState(NULL);
		if(keys[SDLK_ESCAPE]) {
			done = true;
		}

		if(keys[SDLK_g]) {
			show_grid = true;
		}

		if(keys[SDLK_h]) {
			show_grid = false;
		}

		for(int n = SDLK_0; n <= SDLK_9; ++n) {
			if(keys[n]) {
				radius = n - SDLK_0;
			}
		}

		int adjust = 0;
		if(SDL_GetMouseState(NULL,NULL)&SDL_BUTTON(1)) {
			adjust = 1;
		} else if(SDL_GetMouseState(NULL,NULL)&SDL_BUTTON(3)) {
			adjust = -1;
			if(pick_mode) {
				picked_loc = selected;
			}
		}

		if(adjust != 0 && map.is_loc_on_map(selected)) {
			if(new_mutation) {
				new_mutation = false;
				undo_stack.push(undo_info());
			}

			assert(!undo_stack.empty());
			undo_info& undo = undo_stack.top();

			for(int n = 0; n <= radius; ++n) {
				std::vector<hex::location> locs;
				hex::get_tile_ring(selected, n, locs);
				foreach(const hex::location& loc, locs) {
					if(map.is_loc_on_map(loc)) {
						if(undo.locs.count(loc) == 0) {
							undo.tiles.push_back(map.get_tile(loc));
							undo.locs.insert(loc);
						}

						if(pick_mode) {
							if(adjust == 1 && map.is_loc_on_map(picked_loc)) {
								const hex::tile& src = map.get_tile(picked_loc);
								const hex::tile& dst = map.get_tile(loc);
								map.adjust_height(loc,src.height() - dst.height());
								map.set_terrain(loc,src.terrain()->id());
								hex::const_terrain_feature_ptr f = src.feature();
								std::string feature_id;
								if(f) {
									feature_id = f->id();
								}

								map.set_feature(loc,feature_id);
							}
						} else if(!current_feature.empty()) {
							map.set_feature(loc,current_feature);
						} else if(current_terrain.empty()) {
							map.adjust_height(loc,adjust);
						} else {
							map.set_terrain(loc,current_terrain);
						}
					}
				}
			}
		} else {
			new_mutation = true;
		}

		camera.keyboard_control();
	}
}
