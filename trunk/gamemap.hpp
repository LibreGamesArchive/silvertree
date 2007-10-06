
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef GAMEMAP_HPP_INCLUDED
#define GAMEMAP_HPP_INCLUDED

#include <string>
#include <vector>
#include <GL/gl.h>

#include "tile.hpp"
#include "tile_logic.hpp"

namespace hex
{

class gamemap
{
public:
	explicit gamemap(const std::string& data);
	gamemap(const std::vector<tile>& tiles, const location& dim);
	
	std::string write() const;

	const location& size() const { return dim_; }
	const tile& get_tile(const location& loc) const;
	tile& get_tile(const location& loc);
	bool is_loc_on_map(const location& loc) const;

	const tile* closest_tile(GLfloat* x, GLfloat* y) const;

	struct parse_error {
		parse_error(const std::string& msg)
		{}
	};

	void draw() const;
	void draw_grid() const;

	const std::vector<tile>& tiles() const { return map_; }

	//map mutation functions
	void adjust_height(const hex::location& loc, int adjust);
	void set_terrain(const hex::location& loc, const std::string& terrain_id);
	void set_feature(const hex::location& loc, const std::string& feature_id);
	
private:

	void parse(const std::string& data);
	void init_tiles();
	
	std::vector<tile> map_;
	location dim_;
};

bool line_of_sight(const gamemap& m,
                   const location& a, const location& b,
                   std::vector<const tile*>* tiles=NULL,
				   int range=-1, bool draw=false);
		
}

#endif
