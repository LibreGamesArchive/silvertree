
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <algorithm>
#include <cmath>

#include <iostream>

#include "boost/lexical_cast.hpp"

#include "tile_logic.hpp"
#include "util.hpp"
#include "wml_node.hpp"

namespace hex
{

using namespace util;

namespace {
const std::string abbrev[] = {"n","ne","se","s","sw","nw",""};
}

wml::node_ptr write_location(const std::string& name, const location& loc)
{
	wml::node_ptr res(new wml::node(name));
	res->set_attr("x", boost::lexical_cast<std::string>(loc.x()));
	res->set_attr("y", boost::lexical_cast<std::string>(loc.y()));
	return res;
}

wml::node_ptr write_src_dst_location(const std::string& name, const location& src, const location& dst)
{
	wml::node_ptr res(new wml::node(name));
	res->set_attr("xsrc", boost::lexical_cast<std::string>(src.x()));
	res->set_attr("ysrc", boost::lexical_cast<std::string>(src.y()));
	res->set_attr("xdst", boost::lexical_cast<std::string>(dst.x()));
	res->set_attr("ydst", boost::lexical_cast<std::string>(dst.y()));
	return res;
}

variant location::get_value(const std::string& key) const
{
	if(key == "x") {
		return variant(x_);
	} else if(key == "y") {
		return variant(y_);
	} else if(key == "valid") {
		return variant(valid());
	} else {
		return variant();
	}
}

const std::string& direction_abbreviation(DIRECTION dir)
{
	return abbrev[dir];
}

unsigned int distance_between(const location& a, const location& b)
{
	const unsigned int hdistance = std::abs(a.x() - b.x());

	const unsigned int vpenalty = ( (is_even(a.x()) && is_odd(b.x()) && (a.y() < b.y()))
		|| (is_even(b.x()) && is_odd(a.x()) && (b.y() < a.y())) ) ? 1 : 0;

	// for any non-negative integer i, i - i/2 - i%2 == i/2
	// previously returned (hdistance + vdistance - vsavings)
	// = hdistance + vdistance - minimum(vdistance,hdistance/2+hdistance%2)
	// = maximum(hdistance, vdistance+hdistance-hdistance/2-hdistance%2)
	// = maximum(hdistance,abs(a.y-b.y)+vpenalty+hdistance/2)

	return std::max<int>(hdistance, std::abs(a.y() - b.y()) + vpenalty + hdistance/2);
}

void get_adjacent_tiles(const location& a, location* res)
{
	*res = tile_in_direction(a,NORTH);
	++res;
	*res = tile_in_direction(a,NORTH_EAST);
	++res;
	*res = tile_in_direction(a,SOUTH_EAST);
	++res;
	*res = tile_in_direction(a,SOUTH);
	++res;
	*res = tile_in_direction(a,SOUTH_WEST);
	++res;
	*res = tile_in_direction(a,NORTH_WEST);
}

location tile_in_direction(const location& a, DIRECTION dir)
{
	switch(dir) {
		case NORTH:
			return location(a.x(),a.y()-1);
		case NORTH_EAST:
			return location(a.x()+1,a.y() - (is_even(a.x()) ? 1:0));
		case SOUTH_EAST:
			return location(a.x()+1,a.y() + (is_odd(a.x()) ? 1:0));
		case SOUTH:
			return location(a.x(),a.y()+1);
		case SOUTH_WEST:
			return location(a.x()-1,a.y() + (is_odd(a.x()) ? 1:0));
		case NORTH_WEST:
			return location(a.x()-1,a.y() - (is_even(a.x()) ? 1:0));
		default:
			assert(false);
			return location();
	}
}

void get_tile_ring(const location& a, int radius,
                   std::vector<location>& res)
{
	if(radius == 0) {
		res.push_back(a);
		return;
	}
	
	location loc(a);
	for(int i = 0; i != radius; ++i) {
		loc = tile_in_direction(loc,SOUTH_WEST);
	}

	for(int n = 0; n != 6; ++n) {
		const DIRECTION dir = static_cast<DIRECTION>(n);
		for(int i = 0; i != radius; ++i) {
			res.push_back(loc);
			loc = tile_in_direction(loc,dir);
		}
	}
}

void get_tiles_in_radius(const location& center, int radius,
                   std::vector<location>& res)
{
	res.push_back(center);
	for(int n = 1; n <= radius; ++n) {
		get_tile_ring(center, n, res);
	}
}

void get_tile_strip(const location& center, DIRECTION dir,
                    int tiles_forward, int tiles_back, int tiles_side,
					std::vector<location>& res)
{
	location loc = center;
	const DIRECTION reverse_dir = static_cast<DIRECTION>((int(dir)+3)%6);
	for(int n = 0; n != tiles_back; ++n) {
		loc = tile_in_direction(loc, reverse_dir);
	}
	
	const DIRECTION left[] = {static_cast<DIRECTION>((int(dir)+4)%6),
	                          static_cast<DIRECTION>((int(dir)+5)%6)};
	const DIRECTION right[] = {static_cast<DIRECTION>((int(dir)+1)%6),
	                           static_cast<DIRECTION>((int(dir)+2)%6)};
	for(int n = 0; n != tiles_side; ++n) {
		loc = tile_in_direction(loc, left[n%2]);
	}

	const int length = tiles_forward + tiles_back;

	res.push_back(loc);
	for(int n = 0; n != length; ++n) {
		res.push_back(tile_in_direction(res.back(), dir));
	}

	for(int n = 0; n < tiles_side*2; ++n) {
		const int begin = res.size() - length;
		const int end = res.size();
		for(int m = begin; m != end; ++m) {
			res.push_back(tile_in_direction(res[m], right[n%2]));
		}
	}
}

DIRECTION get_adjacent_direction(const location& from, const location& to)
{
	location adj[6];
	get_adjacent_tiles(from,adj);
	for(unsigned int n = 0; n != 6; ++n) {
		if(adj[n] == to) {
			return static_cast<DIRECTION>(n);
		}
	}

	return NULL_DIRECTION;
}

DIRECTION get_main_direction(const location& a, const location& b)
{
	if(tiles_adjacent(a,b)) {
		return get_adjacent_direction(a, b);
	}

	const unsigned int hdistance = std::abs(a.x() - b.x());

	const unsigned int vpenalty = ( (is_even(a.x()) && is_odd(b.x()) && (a.y() < b.y()))
		|| (is_even(b.x()) && is_odd(a.x()) && (b.y() < a.y())) ) ? 1 : 0;
	const unsigned int vdistance = std::abs(a.y() - b.y()) + vpenalty;
	if(hdistance > vdistance) {
		if(a.x() < b.x()) {
			if(a.y() < b.y()) {
				return SOUTH_EAST;
			} else {
				return NORTH_EAST;
			}
		} else {
			if(a.y() < b.y()) {
				return SOUTH_WEST;
			} else {
				return NORTH_WEST;
			}
		}
	} else {
		if(a.y() < b.y()) {
			return SOUTH;
		} else {
			return NORTH;
		}
	}
}

bool tiles_adjacent(const location& a, const location& b)
{
	//two tiles are adjacent if y is different by 1, and x by 0, or if
	//x is different by 1 and y by 0, or if x and y are each different by 1,
	//and the x value of the hex with the greater y value is even

	const int xdiff = std::abs(a.x() - b.x());
	const int ydiff = std::abs(a.y() - b.y());
	return ydiff == 1 && a.x() == b.x() || xdiff == 1 && a.y() == b.y() ||
	       xdiff == 1 && ydiff == 1 && (a.y() > b.y() ? is_even(a.x()) : is_even(b.x()));
}
	
}
