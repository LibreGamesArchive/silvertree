
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef CHARACTER_STATUS_DIALOG_HPP_INCLUDED
#define CHARACTER_STATUS_DIALOG_HPP_INCLUDED

#include "character_fwd.hpp"
#include "dialog.hpp"
#include "party_fwd.hpp"
#include "widget.hpp"

namespace game_dialogs {

class character_status_dialog : public gui::dialog
{
public:
	character_status_dialog(game_logic::character_ptr c,
	                        game_logic::party_ptr p);

	void change_character(int index);
	void improve_attribute(int index);
	void learn_skill();
private:
	bool handle_event(const SDL_Event& event);
	void init();
	game_logic::character_ptr char_;
	game_logic::party_ptr party_;
	gui::widget_ptr modify_char_widget_;
	int equipment_change_;
};

}

#endif
