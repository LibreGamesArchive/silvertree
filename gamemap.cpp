
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "base_terrain.hpp"
#include "gamemap.hpp"
#include "string_utils.hpp"
#include "terrain_feature.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <sstream>

namespace hex
{

namespace {
const int cliff_height = 10;
}

gamemap::gamemap(const std::string& data)
{
	parse(data);
}

gamemap::gamemap(const std::vector<tile>& tiles,
                 const location& dim)
  : map_(tiles), dim_(dim)
{
	assert(dim_.x()*dim_.y() == map_.size());
	init_tiles();
}

const tile& gamemap::get_tile(const location& loc) const
{
	const unsigned int index = loc.y()*dim_.x() + loc.x();
	assert(index < map_.size());
	return map_[index];
}

tile& gamemap::get_tile(const location& loc)
{
	const unsigned int index = loc.y()*dim_.x() + loc.x();
	assert(index < map_.size());
	return map_[index];
}

bool gamemap::is_loc_on_map(const location& loc) const
{
	return loc.x() >= 0 && loc.y() >= 0 &&
	       loc.x() < dim_.x() && loc.y() < dim_.y();
}

const tile* gamemap::closest_tile(GLfloat* xloc, GLfloat* yloc) const
{
	GLfloat& x = *xloc;
	GLfloat& y = *yloc;
	const GLfloat XRatio = 0.86602540378443427;
	x /= XRatio;

	int xint = static_cast<int>(x);
	if(x - xint > 0.5) {
		x += 1.0;
		++xint;
	}

	if((xint%2) == 1) {
		y -= 0.5;
	}


	int yint = static_cast<int>(y);
	if(y - yint > 0.5) {
		y += 1.0;
		++yint;
	}

	if(xint < 0 || yint < 0 || xint >= dim_.x() || yint >= dim_.y()) {
		return NULL;
	}

	const unsigned int index = yint*dim_.x() + xint;
	return &map_[index];
}

std::string gamemap::write() const
{
	std::ostringstream s;
	for(unsigned int n = 0; n != map_.size(); ++n) {
		s << " " << map_[n].height() << " " << map_[n].terrain()->id();

		if(map_[n].feature()) {
			s << " " << map_[n].feature()->id();
		}

		if(((n+1)%dim_.x()) == 0) {
			s << "\n";
		} else {
			s << ",";
		}
	}

	return s.str();
}

void gamemap::parse(const std::string& data)
{
	map_.clear();

	int ncolumns = -1;
	std::vector<std::string> lines = util::split(data,'\n');
	for(std::vector<std::string>::const_iterator line = lines.begin();
	    line != lines.end(); ++line) {
		std::vector<std::string> items = util::split(*line,',');
		if(ncolumns != -1 && size_t(ncolumns) != items.size()) {
			throw parse_error("inconsistent number of tiles in rows");
		}

		ncolumns = items.size();

		for(std::vector<std::string>::const_iterator i = items.begin();
		    i != items.end(); ++i) {
			const location loc(i - items.begin(),line - lines.begin());
			map_.push_back(tile(loc,*i));
		}
	}

	dim_ = location(ncolumns,lines.size());

	init_tiles();
}

void gamemap::init_tiles()
{
	for(std::vector<tile>::iterator i = map_.begin();
	    i != map_.end(); ++i) {
		location adj[6];
		get_adjacent_tiles(i->loc(),adj);
		for(int n = 0; n != 6; ++n) {
			if(is_loc_on_map(adj[n])) {
				const tile& adj_tile = get_tile(adj[n]);
				DIRECTION dir = static_cast<DIRECTION>(n);
				const int height_diff = i->height() - adj_tile.height();
				if(height_diff > cliff_height) {
					i->set_cliff(dir,&adj_tile);
				} else if(height_diff >= -cliff_height) {
					i->set_neighbour(dir,&adj_tile);
				}
			}
		}
	}

	for(std::vector<tile>::iterator i = map_.begin();
	    i != map_.end(); ++i) {
		i->init_corners();
	}

	for(std::vector<tile>::iterator i = map_.begin();
	    i != map_.end(); ++i) {
		i->init_normals();
	}
}

void gamemap::draw() const
{
	for(std::vector<tile>::const_iterator i = map_.begin();
	    i != map_.end(); ++i) {
		i->draw();
	}
}

void gamemap::draw_grid() const
{
	for(std::vector<tile>::const_iterator i = map_.begin();
	    i != map_.end(); ++i) {
		i->draw_grid();
	}
}

void gamemap::adjust_height(const hex::location& loc, int adjust)
{
	const unsigned int index = loc.y()*dim_.x() + loc.x();
	assert(index < map_.size());
	map_[index].adjust_height(adjust);
	hex::location locs[7];
	get_adjacent_tiles(loc,&locs[1]);
	locs[0] = loc;
	const unsigned int main_tile_index = loc.y()*dim_.x() + loc.x();
	tile& main_tile = map_[main_tile_index];
	for(int n = 1; n != 7; ++n) {
		if(is_loc_on_map(locs[n])) {
			tile& adj_tile = get_tile(locs[n]);
			DIRECTION dir = static_cast<DIRECTION>(n-1);
			const int height_diff = main_tile.height() - adj_tile.height();
			if(height_diff > cliff_height) {
				main_tile.set_cliff(dir,&adj_tile);
				main_tile.set_neighbour(dir,NULL);
				DIRECTION adj_dir = static_cast<DIRECTION>((dir+3)%6);
				adj_tile.set_neighbour(adj_dir,NULL);
			} else if(height_diff >= -cliff_height) {
				main_tile.set_neighbour(dir,&adj_tile);
				main_tile.set_cliff(dir,NULL);
			}
		}
	}

	for(int n = 0; n != 7; ++n) {
		if(is_loc_on_map(locs[n])) {
			const unsigned int index = locs[n].y()*dim_.x() + locs[n].x();
			tile& t = map_[index];
			t.invalidate();
		}
	}

	for(int n = 0; n != 7; ++n) {
		if(is_loc_on_map(locs[n])) {
			const unsigned int index = locs[n].y()*dim_.x() + locs[n].x();
			tile& t = map_[index];
			t.init_corners();
		}
	}

	for(int n = 0; n != 7; ++n) {
		if(is_loc_on_map(locs[n])) {
			const unsigned int index = locs[n].y()*dim_.x() + locs[n].x();
			tile& t = map_[index];
			t.init_normals();
		}
	}
}

void gamemap::set_terrain(const hex::location& loc, const std::string& terrain_id)
{
	const unsigned int index = loc.y()*dim_.x() + loc.x();
	assert(index < map_.size());
	map_[index].set_terrain(terrain_id);
	hex::location adj[6];
	get_adjacent_tiles(loc,adj);
	for(int n = 0; n != 6; ++n) {
		if(is_loc_on_map(adj[n])) {
			const unsigned int index = adj[n].y()*dim_.x() + adj[n].x();
			assert(index < map_.size());
			map_[index].invalidate();
		}
	}
}

void gamemap::set_feature(const hex::location& loc, const std::string& feature_id)
{
	const unsigned int index = loc.y()*dim_.x() + loc.x();
	assert(index < map_.size());
	map_[index].set_feature(feature_id);
}

namespace {
void draw_line_of_sight(GLfloat x1, GLfloat y1, GLfloat z1,
                        GLfloat x2, GLfloat y2, GLfloat z2,
                        GLfloat x3, GLfloat y3, GLfloat z3,
						bool out_of_range=false) {
	glDisable(GL_TEXTURE_2D);
	glBegin(GL_LINES);
	glColor4f(0.0,0.0,1.0,1.0);
	glVertex3f(x1,y1,z1);
	glVertex3f(x2,y2,z2);
	if(out_of_range) {
		glColor4f(1.0,1.0,0.0,1.0);
	} else {
		glColor4f(1.0,0.0,0.0,1.0);
	}
	glVertex3f(x2,y2,z2);
	glVertex3f(x3,y3,z3);
	glEnd();
	glEnable(GL_TEXTURE_2D);
}
}

bool line_of_sight(const gamemap& m,
                   const location& a, const location& b,
				   std::vector<const tile*>* tiles,
				   int range, bool draw)
{
	if(!m.is_loc_on_map(a) || !m.is_loc_on_map(b)) {
		return false;
	}

	const tile& t1 = m.get_tile(a);
	const tile& t2 = m.get_tile(b);

	const GLfloat x1 = tile::translate_x(a);
	const GLfloat y1 = tile::translate_y(a);
	const GLfloat h1 = tile::translate_height(t1.height()+3);

	const GLfloat x2 = tile::translate_x(b);
	const GLfloat y2 = tile::translate_y(b);
	const GLfloat h2 = tile::translate_height(t2.height()+3);

	const GLfloat distance = std::sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));

	{
	GLfloat x = x1;
	GLfloat y = y1;
	assert(m.closest_tile(&x,&y) == &t1);
	x = x2;
	y = y2;
	assert(m.closest_tile(&x,&y) == &t2);
	}

	const GLfloat xdiff = x2 - x1;
	const GLfloat ydiff = y2 - y1;
	const GLfloat dist = std::sqrt(xdiff*xdiff + ydiff*ydiff);
	const GLfloat increment = 0.5/dist;

	assert(increment > 0.0);
	for(GLfloat step = 0.0; step < 1.0; step += increment) {
		const GLfloat scale1 = (1.0 - step);
		const GLfloat scale2 = step;
		GLfloat x = x1*scale1 + x2*scale2;
		GLfloat y = y1*scale1 + y2*scale2;
		const GLfloat h = h1*scale1 + h2*scale2;

		GLfloat xloc = x;
		GLfloat yloc = y;

		const tile* t = m.closest_tile(&xloc,&yloc);
		if(t == NULL || range != -1 && distance*step > range) {
			if(draw) {
				draw_line_of_sight(x1,y1,h1,x,y,h,x2,y2,h2,true);
			}
			return false;
		}

		if(t == &t2) {
			if(draw) {
				draw_line_of_sight(x1,y1,h1,x2,y2,h2,x2,y2,h2);
			}
			return true;
		}

		if(tiles != NULL && (tiles->empty() || tiles->back() != t)) {
			tiles->push_back(t);
		}


		if(t->height_at_point_vision(x,y) > h) {
			if(draw) {
				draw_line_of_sight(x1,y1,h1,x,y,h,x2,y2,h2);
			}
			return false;
		}
	}

	if(draw) {
		draw_line_of_sight(x1,y1,h1,x2,y2,h2,x2,y2,h2,draw);
	}
	return true;
}

}
