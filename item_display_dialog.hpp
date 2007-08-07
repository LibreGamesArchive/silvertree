
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef EQUIPMENT_DISPLAY_DIALOG_HPP_INCLUDED
#define EQUIPMENT_DISPLAY_DIALOG_HPP_INCLUDED

#include "dialog.hpp"
#include "item.hpp"

namespace game_dialogs {

class item_display_dialog : public gui::dialog
{
public:
	explicit item_display_dialog(const game_logic::const_item_ptr i);
private:
	game_logic::const_item_ptr item_;
};

}

#endif
