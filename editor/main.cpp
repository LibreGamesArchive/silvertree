
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

#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QMenu>
#include <QtGui/QScrollArea>

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

#include "editormainwindow.hpp"

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
	QApplication app(argc, argv);

	if((argc != 1) && (argc != 2)) {
		std::cerr << "usage: " << argv[0] << " [<mapname>]\n";
		return 0;
	}

	wml::node_ptr game_cfg;

	{
		const int fd = open("data/terrain.cfg",O_RDONLY);
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

	EditorMainWindow *mainWindow = new EditorMainWindow();
	if (argc == 2) {
		mainWindow->openScenario(argv[1]);
	}
	mainWindow->resize(1024,768);
	mainWindow->show();
	app.exec();
	return 0;
}
