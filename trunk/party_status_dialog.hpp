
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef PARTY_STATUS_DIALOG_HPP_INCLUDED
#define PARTY_STATUS_DIALOG_HPP_INCLUDED

#include "dialog.hpp"
#include "party_fwd.hpp"
#include "widget.hpp"

namespace game_dialogs {

class party_status_dialog : public gui::dialog
{
public:
	explicit party_status_dialog(game_logic::party_ptr p);
	void show_character(int index);
private:
	game_logic::party_ptr party_;
};

}

#endif
