
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
#include "character_equip_dialog.hpp"
#include "equipment.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "item_display_dialog.hpp"
#include "label.hpp"
#include "learn_skills_dialog.hpp"
#include "party.hpp"
#include "skill.hpp"
#include "translate.hpp"

namespace game_dialogs {

namespace {

class change_equipment_callback : public functional::callback<character_equip_dialog>
{
	int index_;
	void execute(character_equip_dialog* ptr) { ptr->change_equipment(index_); }
public:
	change_equipment_callback(character_equip_dialog* ptr, int index) : functional::callback<character_equip_dialog>(ptr), index_(index)
	{}
};

class change_character_callback : public functional::callback<character_equip_dialog>
{
	int index_;
	void execute(character_equip_dialog* ptr) { ptr->change_character(index_); }
public:
	change_character_callback(character_equip_dialog* ptr, int index) : functional::callback<character_equip_dialog>(ptr), index_(index)
	{}
};

class preview_equipment_callback : public functional::callback_arg1<character_equip_dialog,int>
{
	void execute(character_equip_dialog* ptr, const int& index) { std::cerr << "preview cb: " << index << "\n"; ptr->preview_equipment(index); }
public:
	explicit preview_equipment_callback(character_equip_dialog* ptr) : functional::callback_arg1<character_equip_dialog,int>(ptr)
	{}
};

class implement_equipment_callback : public functional::callback_arg1<character_equip_dialog,int>
{
	void execute(character_equip_dialog* ptr, const int& index) { ptr->implement_equipment_change(index); }
public:
	explicit implement_equipment_callback(character_equip_dialog* ptr) : functional::callback_arg1<character_equip_dialog,int>(ptr)
	{}
};


}

character_equip_dialog::character_equip_dialog(
     game_logic::character_ptr c, game_logic::party_ptr p)
  : gui::dialog(0,0,graphics::screen_width(),graphics::screen_height()), char_(c), party_(p),
    equipment_change_(-1)
{
	init();
}


void character_equip_dialog::preview_equipment(int index)
{
}

void character_equip_dialog::implement_equipment_change(int index)
{
	remove_widget(modify_char_widget_);
	modify_char_widget_.reset();

	if(index == -1) {
		equipment_change_ = -1;
		return;
	}

	if(index == 0) {
		game_logic::item_ptr it(new game_logic::equipment(
		      char_->equipment()[equipment_change_]->type()));
		char_->swap_equipment(equipment_change_,it);
		if(!it->is_null()) {
			party_->acquire_item(it);
		}

		assert(char_->equipment()[equipment_change_]);
	} else {
		int item_num = 0;
		foreach(game_logic::item_ptr i, party_->inventory()) {
			if(i->type() == char_->equipment()[equipment_change_]->type()) {
				if(--index == 0) {
					break;
				}
			}
			++item_num;
		}

		assert(index == 0);
		party_->assign_equipment(char_,equipment_change_,item_num);
	}

	equipment_change_ = -1;
	clear();
	init();
}

void character_equip_dialog::init()
{
	clear();

	game_logic::character_ptr c = char_;
	game_logic::party_ptr p = party_;

	using namespace gui;
	using functional::callback_ptr;
	typedef widget_ptr ptr;
	const SDL_Color color = {0xFF,0xFF,0,0xFF};
	set_padding(30);

	add_widget(ptr(new image_widget(c->portrait(), 120, 120)), 20, 20,
	           MOVE_RIGHT);

	add_widget(ptr(new label(c->description(), color, 28)));

	const int sz = 22;
	label_factory lb(color, sz);


	const std::vector<game_logic::item_ptr>& equip = c->equipment();
	grid_ptr grid(new gui::grid(equip.size()));
	int index = 0;
	foreach(const game_logic::item_ptr& item, equip) {
		dialog* d = new item_display_dialog(item);
		functional::callback_ptr callback(
		             new change_equipment_callback(this,index++));
		button_ptr b(new button(ptr(new label("Change", color)),
		                        callback));
		d->add_widget(b);
		grid->add_col(widget_ptr(d));
	}

	add_widget(grid,MOVE_DOWN);

	widget_ptr close_label(new label("Close",color));
	callback_ptr close_callback(new gui::close_dialog_callback(this));
	button_ptr close_button(new button(close_label,close_callback));
	add_widget(close_button, 1024-100, 800-100);

	index = 0;
	foreach(const game_logic::character_ptr& ch, party_->members()) {
		callback_ptr char_callback(new change_character_callback(this,index));
		widget_ptr image(new image_widget(ch->portrait(),100,100));
		button_ptr char_button(new button(image,char_callback));
		if(index == 0) {
			add_widget(char_button, 0, height()-char_button->height(), MOVE_RIGHT);
		} else {
			add_widget(char_button);
		}
		++index;
	}
}

void character_equip_dialog::change_equipment(int index)
{
	if(!party_) {
		return;
	}

	remove_widget(modify_char_widget_);

	SDL_Color color = {0xFF,0xFF,0xFF,0xFF};

	using namespace gui;
	typedef gui::widget_ptr ptr;
	gui::grid* grid = new gui::grid(2);
	modify_char_widget_.reset(grid);
	grid->add_col(ptr(new label("", color)));
	grid->add_col(ptr(new label("None", color)));
	equipment_change_ = index;
	foreach(const game_logic::item_ptr& i, party_->inventory()) {
		if(i->type() == char_->equipment()[index]->type()) {
			grid->add_col(ptr(new image_widget(i->image(), 50, 50)));
			grid->add_col(ptr(new label(i->description(), color)));
		}
	}

	grid->allow_selection();
	grid->register_mouseover_callback(gui::grid::select_callback_ptr(
	                        new preview_equipment_callback(this)));
	grid->register_selection_callback(gui::grid::select_callback_ptr(
							new implement_equipment_callback(this)));

	int mousex, mousey;
	SDL_GetMouseState(&mousex,&mousey);

	if(mousex+grid->width() > graphics::screen_width()) {
		mousey = graphics::screen_width() - grid->width();
	}

	if(mousey+grid->height() > graphics::screen_height()) {
		mousey = graphics::screen_height() - grid->height();
	}

	add_widget(modify_char_widget_, mousex, mousey);
}


bool character_equip_dialog::handle_event(const SDL_Event& event)
{
	if(modify_char_widget_) {

		//make sure that if the event ends up removing the
		//widget we don't delete it til the end of the event handling
		gui::widget_ptr ptr = modify_char_widget_;
		SDL_Event ev = event;
		normalize_event(&ev);
		return ptr->process_event(ev);
	}
	return dialog::handle_event(event);
}

void character_equip_dialog::change_character(int index)
{
	char_ = party_->members()[index%party_->members().size()];
	init();
}

}
