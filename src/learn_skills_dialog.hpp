
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef LEARN_SKILLS_DIALOG_HPP_INCLUDED
#define LEARN_SKILLS_DIALOG_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

#include <vector>

#include "character_fwd.hpp"
#include "dialog.hpp"
#include "grid_widget.hpp"
#include "skill.hpp"

namespace game_dialogs {

class skill_dialog;

class learn_skills_dialog : public gui::dialog
{
public:
	explicit learn_skills_dialog(game_logic::character_ptr c);

	void next_page();

private:
	void init();
	bool handle_event(const SDL_Event& event);
	gui::grid_ptr get_grid_for_skill(game_logic::const_skill_ptr s);
	game_logic::character_ptr char_;

	std::vector<boost::shared_ptr<skill_dialog> > children_;
	int page_, page_size_;
};

}

#endif
