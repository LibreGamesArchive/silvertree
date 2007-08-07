
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TILE_LOGIC_HPP_INCLUDED
#define TILE_LOGIC_HPP_INCLUDED

#include <string>
#include <vector>

namespace hex
{
	class location {
		int x_, y_;
	public:
		location() : x_(-1), y_(-1)
		{}

		location(int x, int y) : x_(x), y_(y)
		{}

		int x() const { return x_; }
		int y() const { return y_; }

		bool valid() const { return x_ >= 0 && y_ >= 0; }
	};

	enum DIRECTION {
		NORTH, NORTH_EAST, SOUTH_EAST, SOUTH, SOUTH_WEST, NORTH_WEST,
		NULL_DIRECTION
	};

	const std::string& direction_abbreviation(DIRECTION dir);

	bool tiles_adjacent(const location& a, const location& b);
	void get_adjacent_tiles(const location& a, location* res);
	location tile_in_direction(const location& a, DIRECTION dir);

	void get_tile_ring(const location& a, int radius,
	                   std::vector<location>& res);

	DIRECTION get_adjacent_direction(const location& a, const location& b);
	
	unsigned int distance_between(const location& a, const location& b);

	inline bool operator==(const location& a, const location& b)
	{
		return a.x() == b.x() && a.y() == b.y();
	}
	
	inline bool operator!=(const location& a, const location& b)
	{
		return !(a == b);
	}

	inline bool operator<(const location& a, const location& b)
	{
		return a.x() < b.x() || a.x() == b.x() && a.y() < b.y();
	}
}


#endif
