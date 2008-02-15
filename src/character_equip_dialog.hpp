
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef CHARACTER_EQUIP_DIALOG_HPP_INCLUDED
#define CHARACTER_EQUIP_DIALOG_HPP_INCLUDED

#include "character_fwd.hpp"
#include "dialog.hpp"
#include "party_fwd.hpp"
#include "widget.hpp"

namespace game_dialogs {

class character_equip_dialog : public gui::dialog
{
public:
	character_equip_dialog(game_logic::character_ptr c,
	                        game_logic::party_ptr p);

	void change_equipment(int index);
	void preview_equipment(int index);
	void implement_equipment_change(int index);
	void change_character(int index);
private:
	bool handle_event(const SDL_Event& event, bool claimed);
	void init();
	game_logic::character_ptr char_;
	game_logic::party_ptr party_;
	gui::widget_ptr modify_char_widget_;
	int equipment_change_;
};

}

#endif
