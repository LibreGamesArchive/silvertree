
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

class change_equipment_callback : public functional::callback<character_status_dialog>
{
	int index_;
	void execute(character_status_dialog* ptr) { ptr->change_equipment(index_); }
public:
	change_equipment_callback(character_status_dialog* ptr, int index) : functional::callback<character_status_dialog>(ptr), index_(index)
	{}
};

class change_character_callback : public functional::callback<character_status_dialog>
{
	int index_;
	void execute(character_status_dialog* ptr) { ptr->change_character(index_); }
public:
	change_character_callback(character_status_dialog* ptr, int index) : functional::callback<character_status_dialog>(ptr), index_(index)
	{}
};

class improve_attribute_callback : public functional::callback<character_status_dialog>
{
	int index_;
	void execute(character_status_dialog* ptr) { ptr->improve_attribute(index_); }
public:
	improve_attribute_callback(character_status_dialog* ptr, int index) : functional::callback<character_status_dialog>(ptr), index_(index)
	{}
};

class preview_equipment_callback : public functional::callback_arg1<character_status_dialog,int>
{
	void execute(character_status_dialog* ptr, const int& index) { std::cerr << "preview cb: " << index << "\n"; ptr->preview_equipment(index); }
public:
	explicit preview_equipment_callback(character_status_dialog* ptr) : functional::callback_arg1<character_status_dialog,int>(ptr)
	{}
};

class implement_equipment_callback : public functional::callback_arg1<character_status_dialog,int>
{
	void execute(character_status_dialog* ptr, const int& index) { ptr->implement_equipment_change(index); }
public:
	explicit implement_equipment_callback(character_status_dialog* ptr) : functional::callback_arg1<character_status_dialog,int>(ptr)
	{}
};

class add_skill_callback : public functional::callback<character_status_dialog>
{
public:
	explicit add_skill_callback(character_status_dialog* ptr) : functional::callback<character_status_dialog>(ptr)
	{}

private:
	void execute(character_status_dialog* ptr) { ptr->learn_skill(); }
};

}

character_status_dialog::character_status_dialog(
     game_logic::character_ptr c, game_logic::party_ptr p)
  : gui::dialog(0,0,1024,768), char_(c), party_(p),
    equipment_change_(-1)
{
	init();
}

void character_status_dialog::init()
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

	grid_ptr grid(new gui::grid(3));
	grid->add_col(lb.create("level","level_tip")).
	      add_col(lb.create(formatter() << c->level())).
		  add_col(lb.create("")).
		  add_col(lb.create("hp","hp_tip")).
		  add_col(lb.create(formatter() << c->hitpoints() << "/")).
		  add_col(lb.create(formatter() << c->max_hitpoints())).
		  add_col(lb.create("xp","xp_tip")).
		  add_col(lb.create(formatter() << c->experience() << "/")).
		  add_col(lb.create(formatter() << c->experience_required())).
	      add_col(lb.create("fatigue","fatigue_tip")).
		  add_col(lb.create(formatter() << (c->fatigue()/10) << "/")).
		  add_col(lb.create(formatter() << (c->stamina()/10))).
	      add_col(lb.create("speed","speed_tip")).
		  add_col(lb.create(formatter() << c->speed())).
		  add_col(lb.create("")).
	      add_col(lb.create("climb","climb_tip")).
		  add_col(lb.create(formatter() << c->climbing())).
		  add_col(lb.create("")).
		  add_col(lb.create("vision","vision_tip")).
		  add_col(lb.create(formatter() << c->vision())).
		  add_col(lb.create(""));
	grid->set_col_width(0,60).set_col_width(1,60).set_col_width(2,60)
	     .set_align(1,grid::ALIGN_RIGHT).set_align(2,grid::ALIGN_RIGHT);
	add_widget(grid, MOVE_RIGHT);

	grid.reset(new gui::grid(3));
	grid->add_col(lb.create("attack","attack_tip")).
	      add_col(lb.create(formatter() << c->attack())).
		  add_col(lb.create("")).
		  add_col(lb.create("defense","defense_tip")).
	      add_col(lb.create(formatter() << c->defense())).
		  add_col(lb.create("")).
		  add_col(lb.create("  "+i18n::translate("dodge"),"dodge_tip")).
	      add_col(lb.create(formatter() << c->dodge())).
		  add_col(lb.create("")).
		  add_col(lb.create("  "+i18n::translate("parry"),"parry_tip")).
	      add_col(lb.create(formatter() << c->parry())).
		  add_col(lb.create("")).
	      add_col(lb.create("damage","damage_tip")).
	      add_col(lb.create(formatter() << c->damage())).
		  add_col(lb.create("")).
	      add_col(lb.create("initiative","initiative_tip")).
	      add_col(lb.create(formatter() << c->initiative())).
		  add_col(lb.create(""));

	int resistance_amount, resistance_percent;
	c->get_resistance("", &resistance_amount, &resistance_percent);
	grid->add_col(lb.create("resistance","resistance_tip")).
	      add_col(lb.create(formatter() << resistance_amount)).
		  add_col(lb.create(formatter() << "(" << resistance_percent << "%)"));

	const std::string types[] = {"slash", "stab", "crush", "missile"};
	foreach(const std::string& type, types) {
		int amount, percent;
		c->get_resistance(type, &amount, &percent);
		if(percent != resistance_percent) {
			grid->add_col(ptr(new label(formatter() << "  vs " << type, color, sz))).
			      add_col(ptr(new label("",color,sz))).
				  add_col(ptr(new label(formatter() << "(" << percent << "%)",color,sz)));
		}
	}
	
	grid->set_col_width(0,120).set_col_width(1,40).set_col_width(2,60).
		  set_align(1,grid::ALIGN_RIGHT).set_align(2,grid::ALIGN_RIGHT);
	add_widget(grid, MOVE_RIGHT);

	grid.reset(new gui::grid(4));
	int index = 0;
	foreach(const std::string& a, game_logic::character::attributes()) {
		ptr improve(new label("", color, sz));
		if(char_->can_improve_attr(a)) {
			functional::callback_ptr callback(
			                    new improve_attribute_callback(this,index));
			improve.reset(new button(ptr(new label("+", color)),callback));
		}
		grid->add_col(ptr(new label(a, color, sz))).
		      add_col(ptr(new label("", color, sz))).
		      add_col(ptr(new label(formatter() << c->get_attr(a), color, sz))).
		      add_col(improve);
		++index;
	}

	if(c->improvement_points()) {
		grid->add_col(ptr(new label("", color, sz))).
		      add_col(ptr(new label("", color, sz))).
		      add_col(ptr(new label("", color, sz))).
		      add_col(ptr(new label("", color, sz))).
			  add_col(ptr(new label("Points", color, sz))).
		      add_col(ptr(new label("", color, sz))).
			  add_col(ptr(new label(formatter() << c->improvement_points(), color, sz))).
		      add_col(ptr(new label("", color, sz)));
	}

	grid->set_col_width(0,120).set_col_width(1,40).
	      set_col_width(2,40).set_col_width(3,40).
	      set_align(2,grid::ALIGN_RIGHT);
	add_widget(grid);

	grid.reset(new gui::grid(2));
	grid->set_hpad(10);
	grid->add_col(lb.create("skill_points")).
	      add_col(lb.create(formatter() << c->skill_points()));
	add_widget(grid, 20, cursor_y());

	grid.reset(new gui::grid(1));
	grid->set_hpad(10);
	using game_logic::const_skill_ptr;
	const std::vector<const_skill_ptr>& skills = c->skills();
	foreach(const const_skill_ptr& s, skills) {
		grid->add_col(lb.create(s->name(), s->description()));
	}
	add_widget(grid);

	functional::callback_ptr skill_callback(new add_skill_callback(this));
	button_ptr add_skill_button(new button(ptr(new label("Learn Skill", color)), skill_callback));
	add_widget(add_skill_button);

	const std::vector<game_logic::item_ptr>& equip = c->equipment();
	grid.reset(new gui::grid(equip.size()));
	index = 0;
	foreach(const game_logic::item_ptr& item, equip) {
		dialog* d = new item_display_dialog(item);
		functional::callback_ptr callback(
		             new change_equipment_callback(this,index++));
		button_ptr b(new button(ptr(new label("Change", color)),
		                        callback));
		d->add_widget(b);
		grid->add_col(widget_ptr(d));
	}

	add_widget(grid,20,500);

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

void character_status_dialog::handle_event(const SDL_Event& event)
{
	if(modify_char_widget_) {

		//make sure that if the event ends up removing the
		//widget we don't delete it til the end of the event handling
		gui::widget_ptr ptr = modify_char_widget_;
		SDL_Event ev = event;
		normalize_event(&ev);
		ptr->process_event(ev);
		return;
	}
	dialog::handle_event(event);
}

void character_status_dialog::change_equipment(int index)
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

	if(mousex+grid->width() > 1024) {
		mousey = 1024 - grid->width();
	}

	if(mousey+grid->height() > 768) {
		mousey = 768 - grid->height();
	}

	add_widget(modify_char_widget_, mousex, mousey);
}

void character_status_dialog::preview_equipment(int index)
{
}

void character_status_dialog::implement_equipment_change(int index)
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

void character_status_dialog::change_character(int index)
{
	char_ = party_->members()[index%party_->members().size()];
	init();
}

void character_status_dialog::improve_attribute(int index)
{
	std::cerr << "improve attribute " << index << "\n";
	assert(index >= 0 && index < game_logic::character::attributes().size());
	char_->improve_attr(game_logic::character::attributes()[index]);
	init();
}

void character_status_dialog::learn_skill()
{
	learn_skills_dialog d(char_);
	d.show_modal();
	init();
}

}
