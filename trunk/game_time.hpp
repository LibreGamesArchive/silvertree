
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef GAME_TIME_HPP_INCLUDED
#define GAME_TIME_HPP_INCLUDED

#include <string>

#include "formula.hpp"
#include "formula_callable.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class game_time : public formula_callable
{
public:
	game_time();
	explicit game_time(wml::const_node_ptr node);

	int day() const;
	int hour() const;
	int minute() const;

	int since_epoch() const;

	bool valid() const;

	game_time& operator++();
	game_time& operator+=(int mins);

	int alignment_adjustment() const;

private:
	variant get_value(const std::string& key) const;
	int minutes_since_epoch_;
};

bool operator==(const game_time& t1, const game_time& t2);
bool operator!=(const game_time& t1, const game_time& t2);
bool operator<(const game_time& t1, const game_time& t2);
bool operator>(const game_time& t1, const game_time& t2);
bool operator<=(const game_time& t1, const game_time& t2);
bool operator>=(const game_time& t1, const game_time& t2);

int operator-(const game_time& t1, const game_time& t2);
game_time operator+(const game_time& t, int mins);
		
}

#endif
