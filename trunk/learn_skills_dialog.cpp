
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "button.hpp"
#include "callback.hpp"
#include "character.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "learn_skills_dialog.hpp"
#include "skill_dialog.hpp"
#include "translate.hpp"

namespace game_dialogs {

namespace {
using namespace game_logic;
void get_skills_derived_from(const std::string& skill_name,
                             std::vector<const_skill_ptr>* res)
{
	const skill::skills_map& m = skill::all_skills();
	for(skill::skills_map::const_iterator i = m.begin(); i != m.end(); ++i) {
		if(i->second->prerequisite() == skill_name) {
			res->push_back(i->second);
		}
	}
}

DEFINE_CALLBACK(next_page_callback, learn_skills_dialog, next_page);

}

gui::grid_ptr learn_skills_dialog::get_grid_for_skill(const_skill_ptr s)
{
	using namespace gui;
	grid_ptr g(new grid(2));
	boost::shared_ptr<skill_dialog> child(new skill_dialog(char_, s));
	children_.push_back(child);
	g->add_col(child);
	
	std::vector<const_skill_ptr> children;
	get_skills_derived_from(s->name(), &children);
	grid_ptr child_grid(new grid(1));
	foreach(const_skill_ptr child, children) {
		child_grid->add_col(get_grid_for_skill(child));
	}

	g->add_col(child_grid);
	return g;
}

learn_skills_dialog::learn_skills_dialog(game_logic::character_ptr c)
  : gui::dialog(0,0,1024,768), char_(c), page_(0), page_size_(0)
{
	init();
}

void learn_skills_dialog::init()
{
	clear();
	children_.clear();

	using namespace gui;
	using game_logic::skill;
	typedef widget_ptr ptr;
	set_cursor(20,20);
	const SDL_Color color = {0xFF,0xFF,0x0};
	const int sz = 22;
	label_factory lb(color, sz);

	add_widget(ptr(new image_widget(char_->portrait(), 120, 120)), 20, 20, MOVE_RIGHT);
	add_widget(ptr(new label(char_->description(), color, 28)));
	add_widget(ptr(new label(formatter() << i18n::translate("skill_points")
	                                     << ": " << char_->skill_points(), color, sz)));

	set_cursor(20,160);

	const int Bottom = 700;

	std::vector<const_skill_ptr> base_skills;
	get_skills_derived_from("", &base_skills);
	if(page_ >= base_skills.size()) {
		page_ = 0;
	}
	page_size_ = 0;
	for(int n = page_; n < base_skills.size(); ++n) {
		grid_ptr g(get_grid_for_skill(base_skills[n]));
		if(page_size_ && cursor_y()+g->height() > Bottom) {
			break;
		}

		add_widget(g, MOVE_DOWN);
		++page_size_;
	}

	functional::callback_ptr next_callback(new next_page_callback(this));
	button_ptr next_button(new button(label::create("More >>", color), next_callback));
	add_widget(next_button, 1024-100, 800-180);

	widget_ptr close_label(new label("Close",color));
	functional::callback_ptr close_callback(new gui::close_dialog_callback(this));
	button_ptr close_button(new button(close_label,close_callback));
	add_widget(close_button, 1024-100, 800-100);
}

bool learn_skills_dialog::handle_event(const SDL_Event& event)
{
	bool claimed = false;
	claimed = gui::dialog::handle_event(event);
	foreach(const boost::shared_ptr<skill_dialog>& d, children_) {
		if(d->learnt_skill()) {
			init();
			break;
		}
	}
	return claimed;
}

void learn_skills_dialog::next_page()
{
	page_ += page_size_;
	init();
}

}
