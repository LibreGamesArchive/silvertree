
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef POST_BATTLE_DIALOG_HPP_INCLUDED
#define POST_BATTLE_DIALOG_HPP_INCLUDED

#include <vector>

#include "dialog.hpp"
#include "label.hpp"
#include "party_fwd.hpp"

namespace game_dialogs {

class post_battle_dialog : public gui::dialog
{
public:
	post_battle_dialog(game_logic::party_ptr p,
	                   int xp, int money);

	bool iterate();
	void show();

private:
	game_logic::party_ptr party_;
	std::vector<gui::label_ptr> xp_required_;
	std::vector<gui::label_ptr> level_;
	gui::label_ptr money_label_;
	gui::label_ptr xp_left_;
	gui::label_ptr money_left_;
	int xp_;
	int money_;
};

}

#endif
