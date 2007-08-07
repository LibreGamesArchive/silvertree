
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
#include <SDL/SDL.h>

#include <cmath>
#include <iostream>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "camera.hpp"
#include "character.hpp"
#include "filesystem.hpp"
#include "font.hpp"
#include "formula_registry.hpp"
#include "gamemap.hpp"
#include "wml_parser.hpp"
#include "wml_node.hpp"
#include "base_terrain.hpp"
#include "party.hpp"
#include "skill.hpp"
#include "terrain_feature.hpp"
#include "translate.hpp"
#include "world.hpp"

extern "C" int main(int argc, char** argv)
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_NOPARACHUTE) < 0) {
		std::cerr << "could not init SDL\n";
		return -1;
	}

	if(SDL_SetVideoMode(1024,768,0,SDL_OPENGL) == NULL) {
		std::cerr << "could not set video mode\n";
		return -1;
	}

	{
	const std::string data = sys::read_file("translate.cfg");
	if(data.empty() == false) {
		wml::node_ptr cfg(wml::parse_wml(data));
		for(wml::node::const_attr_iterator i = cfg->begin_attr();
		    i != cfg->end_attr(); ++i) {
			i18n::add_translation(i->first,i->second);
		}
	}
	}

	graphics::font::manager font_manager;

	std::string mapdata;

	{
	const int fd = open("map",O_RDONLY);
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
		} else if((*cfg1)->name() == "skill") {
			game_logic::skill::add_skill(*cfg1);
		} else if((*cfg1)->name() == "calculations") {
			formula_registry::load(*cfg1);
		}

		++cfg1;
	}

	GLfloat material_specular[] = {1.0,1.0,1.0,1.0};
	GLfloat material_shininess[] = {1000.0};
	GLfloat intensity = 1.0;
	GLfloat ambient_light[] = {intensity,intensity,intensity,1.0};
	GLfloat diffuse_light[] = {1.0,1.0,1.0,1.0};

	GLfloat dim_ambient[] = {0.5,0.5,0.5,1.0};
	GLfloat dim_diffuse[] = {0.5,0.5,0.5,1.0};

	//glMaterialfv(GL_FRONT,GL_SPECULAR,material_specular);
	//glMaterialfv(GL_FRONT,GL_SHININESS,material_shininess);

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
	glAlphaFunc(GL_GREATER,0.01);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

	game_logic::world w(game_cfg);
	w.play();

	SDL_Quit();
}
