
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SKILL_DIALOG_HPP_INCLUDED
#define SKILL_DIALOG_HPP_INCLUDED

#include "character_fwd.hpp"
#include "dialog.hpp"
#include "skill_fwd.hpp"

namespace game_dialogs {

class skill_dialog : public gui::dialog
{
public:
	skill_dialog(game_logic::character_ptr c, game_logic::const_skill_ptr s);
	void learn();
	bool learnt_skill() const { return learnt_; }
private:
	game_logic::character_ptr char_;
	game_logic::const_skill_ptr skill_;
	bool learnt_;
};
}

#endif
