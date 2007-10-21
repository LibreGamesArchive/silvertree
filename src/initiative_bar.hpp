/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef INITIATIVE_BAR_HPP_INCLUDED
#define INITIATIVE_BAR_HPP_INCLUDED

#include "battle_character_fwd.hpp"
#include "widget.hpp"

#include <vector>

namespace gui {

class initiative_bar : public widget
{
public:
	initiative_bar() : focus_(NULL), add_focus_(0.0), current_time_(0.0)
	{}
	void add_character(const game_logic::const_battle_character_ptr& c);
	void remove_character(const game_logic::battle_character* c);

	void focus_character(const game_logic::battle_character* c, double add=0.0);
	void set_current_time(double time) { current_time_ = time; }
private:
	void handle_draw() const;

	std::vector<game_logic::const_battle_character_ptr> chars_;
	const game_logic::battle_character* focus_;
	double add_focus_;
	double current_time_;
};

}

#endif
