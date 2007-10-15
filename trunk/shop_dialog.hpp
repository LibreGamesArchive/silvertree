
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SHOP_DIALOG_HPP_INCLUDED
#define SHOP_DIALOG_HPP_INCLUDED

#include <vector>

#include "dialog.hpp"
#include "item.hpp"
#include "party_fwd.hpp"

namespace game_dialogs {

class shop_dialog : public gui::dialog
{
public:
	shop_dialog(game_logic::party& party, int cost,
	            const std::string& items);

	void purchase(int index);
private:
	void init();
	int price(const game_logic::const_item_ptr& i) const;
	game_logic::party& party_;
	int cost_;
	std::vector<game_logic::const_item_ptr> items_;
};

}

#endif
