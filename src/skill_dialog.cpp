
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

#include "SDL.h"

#include "button.hpp"
#include "callback.hpp"
#include "character.hpp"
#include "formatter.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "skill.hpp"
#include "skill_dialog.hpp"
#include "translate.hpp"

namespace game_dialogs {

namespace {
const SDL_Color known_skill_color = {0xFF,0xFF,0x0};
const SDL_Color learnable_skill_color = {0x0,0xFF,0x0};
const SDL_Color unreachable_skill_color = {0x4F,0x4F,0x4F};
}

DEFINE_CALLBACK(learn_skill_callback, skill_dialog, learn);

skill_dialog::skill_dialog(game_logic::character_ptr c, game_logic::const_skill_ptr s)
  : gui::dialog(0,0,200,100), char_(c), skill_(s), learnt_(false)
{
	clear();

	using namespace game_logic;
	set_cursor(20,20);
	SDL_Color col = unreachable_skill_color;
	bool learnable = false;
	bool show_difficulty = true;
	if(c->has_skill(s->name())) {
		show_difficulty = false;
		col = known_skill_color;
	} else {
		const std::vector<const_skill_ptr>& eligible = c->eligible_skills();
		if(std::find(eligible.begin(),eligible.end(),s) != eligible.end()) {
			col = learnable_skill_color;
			learnable = true;
		}
	}

	using namespace gui;
	add_widget(label::create(s->name(), col, 22), MOVE_DOWN);
	if(show_difficulty) {
		add_widget(
		label::create(formatter() << i18n::translate("learn_difficulty") << ": "
		                          << skill_->cost(*char_), col, 22), MOVE_DOWN);
	}

	if(learnable) {
		button_ptr b(new button(label::create("learn_skill", graphics::color_yellow()),
		                        functional::callback_ptr(new learn_skill_callback(this))));
		add_widget(b);
	}

	set_tooltip(s->description());
}

void skill_dialog::learn()
{
	char_->learn_skill(skill_->name());
	learnt_ = true;
}

}
