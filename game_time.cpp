
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "gl.h"

#include <iostream>
#include <cmath>

#include "game_time.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace game_logic
{

namespace {

const int HoursInDay = 24;
const int MinutesInHour = 60;
		
}

game_time::game_time() : minutes_since_epoch_(-1)
{}

game_time::game_time(wml::const_node_ptr node)
   : minutes_since_epoch_(wml::get_attr<int>(node,"time"))
{
	if(minutes_since_epoch_ == 0) {
		minutes_since_epoch_ =
		    wml::get_attr<int>(node,"days")*HoursInDay*MinutesInHour +
		    wml::get_attr<int>(node,"hours")*MinutesInHour +
		    wml::get_attr<int>(node,"minutes");
	}
}

int game_time::day() const
{
	return minutes_since_epoch_/(HoursInDay*MinutesInHour);
}

int game_time::hour() const
{
	return (minutes_since_epoch_/MinutesInHour)%HoursInDay;
}

int game_time::minute() const
{
	return minutes_since_epoch_%MinutesInHour;
}

int game_time::since_epoch() const
{
	return minutes_since_epoch_;
}

bool game_time::valid() const
{
	return minutes_since_epoch_ >= 0;
}

game_time& game_time::operator++()
{
	++minutes_since_epoch_;
	return *this;
}

game_time& game_time::operator+=(int mins)
{
	minutes_since_epoch_ += mins;
	return *this;
}

bool operator==(const game_time& t1, const game_time& t2)
{
	return t1.since_epoch() == t2.since_epoch();
}

bool operator!=(const game_time& t1, const game_time& t2)
{
	return t1.since_epoch() != t2.since_epoch();
}

bool operator<(const game_time& t1, const game_time& t2)
{
	return t1.since_epoch() < t2.since_epoch();
}

bool operator>(const game_time& t1, const game_time& t2)
{
	return t2 < t1;
}

bool operator<=(const game_time& t1, const game_time& t2)
{
	return !(t1 > t2);
}

bool operator>=(const game_time& t1, const game_time& t2)
{
	return !(t1 < t2);
}

int operator-(const game_time& t1, const game_time& t2)
{
	return t1.since_epoch() - t2.since_epoch();
}

game_time operator+(const game_time& t, int mins)
{
	game_time res(t);
	res += mins;
	return res;
}

int game_time::get_value(const std::string& key) const
{
	if(key == "day") {
		return day();
	} else if(key == "hour") {
		return hour();
	} else if(key == "minute") {
		return minute();
	} else if(key == "daypercent") {
		return (100*(hour()*60 + minute()))/(24*60);
	} else {
		return 0;
	}
}

int game_time::alignment_adjustment() const
{
	switch(hour()) {
	case 5:
		return -16;
	case 6:
		return -8;
	case 7:
		return 8;
	case 8:
		return 16;
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		return 25;
	case 17:
		return 16;
	case 18:
		return 8;
	case 19:
		return -8;
	case 20:
		return -16;
	case 21:
	case 22:
	case 23:
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		return -25;
	}
}

}
