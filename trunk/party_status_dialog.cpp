
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <iostream>

#include "button.hpp"
#include "callback.hpp"
#include "character.hpp"
#include "character_status_dialog.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "item_display_dialog.hpp"
#include "label.hpp"
#include "party_status_dialog.hpp"
#include "party.hpp"
#include "translate.hpp"

namespace game_dialogs {

namespace {
class show_character_callback : public functional::callback<party_status_dialog>
{
	int index_;
	void execute(party_status_dialog* ptr) { ptr->show_character(index_); }
public:
	show_character_callback(party_status_dialog* ptr, int index) : functional::callback<party_status_dialog>(ptr), index_(index)
	{}
};
}

party_status_dialog::party_status_dialog(game_logic::party_ptr p)
  : gui::dialog(0,0,1024,768), party_(p)
{
	using namespace gui;
	using functional::callback_ptr;
	const SDL_Color color = {0xFF,0xFF,0,0xFF};
	const int sz = 22;
	label_factory lb(color, sz);

	set_padding(30);

	grid_ptr grid(new gui::grid(party_->members().size()));

	int index = 0;
	foreach(const game_logic::character_ptr& ch, party_->members()) {
		widget_ptr image(new image_widget(ch->portrait(),100,100));

		grid_ptr hgrid(new gui::grid(2));
		hgrid->add_col(image);
		grid_ptr stats_grid(new gui::grid(3));
		stats_grid->add_col(lb.create(ch->description())).
					add_col(lb.create("")).
					add_col(lb.create("")).
					add_col(lb.create("level","level_tip")).
		            add_col(lb.create(formatter() << ch->level())).
					add_col(lb.create("")).
					add_col(lb.create("hp","hp_tip")).
					add_col(lb.create(formatter() << ch->hitpoints() << "/")).
					add_col(lb.create(formatter() << ch->max_hitpoints())).
					add_col(lb.create("xp","xp_tip")).
					add_col(lb.create(formatter() << ch->experience() << "/")).
					add_col(lb.create(formatter() << ch->experience_required()));
		hgrid->add_col(stats_grid);
		callback_ptr char_callback(new show_character_callback(this,index));
		button_ptr char_button(new button(hgrid,char_callback));

		grid->add_col(char_button);
		++index;
	}

	add_widget(grid);

	add_widget(lb.create(formatter() << party_->money() << " " <<
	                     i18n::translate("currency_units")));

	add_widget(lb.create("inventory"));

	grid.reset(new gui::grid(3));
	foreach(const game_logic::item_ptr& i, party_->inventory()) {
		widget_ptr w(new item_display_dialog(i));
		grid->add_col(w);
	}

	grid->finish_row();
	add_widget(grid);

	widget_ptr close_label(new label("Close",color));
	callback_ptr close_callback(new gui::close_dialog_callback(this));
	button_ptr close_button(new button(close_label,close_callback));
	add_widget(close_button, 1024-100, 800-100);
}

void party_status_dialog::show_character(int index)
{
	character_status_dialog(party_->members()[index],party_).show_modal();
}

}
