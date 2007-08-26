
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <map>

#include "equipment.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "item_display_dialog.hpp"
#include "label.hpp"

namespace game_dialogs {

using namespace gui;
using game_logic::item;
using game_logic::const_item_ptr;
item_display_dialog::item_display_dialog(const const_item_ptr i)
  : dialog(0,0,300,250), item_(i)
{
	const SDL_Color color = {0xFF,0xFF,0,0xFF};
	const int sz = 22;

	typedef widget_ptr ptr;
	add_widget(ptr(new image_widget(item_->image(),80,80)), 10, 10, MOVE_RIGHT);
	add_widget(ptr(new label(item::type_name(item_->type()), color, 28)));
	add_widget(ptr(new label(item_->description(), color, 28)));

	if(const game_logic::equipment* equip = item_as_equipment(item_)) {
		gui::grid_ptr grid(new gui::grid(2));

		const std::map<std::string,int>& attr = equip->stats();
		typedef std::pair<std::string,int> attr_pair;
		foreach(const attr_pair& a, attr) {
			grid->add_col(ptr(new label(a.first,color,sz))).
			      add_col(ptr(new label(formatter() << a.second,color,sz)));
			grid->set_align(1,grid::ALIGN_RIGHT).set_col_width(1,50);
		}

		add_widget(grid);
	}
}
		
}
