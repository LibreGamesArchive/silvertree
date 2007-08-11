
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BATTLE_HPP_INCLUDED
#define BATTLE_HPP_INCLUDED

#include <set>
#include <vector>

#include "battle_character.hpp"
#include "battle_menu_fwd.hpp"
#include "battle_move_fwd.hpp"
#include "camera.hpp"
#include "camera_controller.hpp"
#include "gamemap.hpp"
#include "texture.hpp"
#include "widget.hpp"

namespace gui
{
class slider;
}

namespace game_logic
{

class battle
{
public:
	battle(const std::vector<battle_character_ptr>& chars,
	       const hex::gamemap& battle_map);

	void play();
	void player_turn(battle_character& c);
	const hex::gamemap& map() const { return map_; }

	void move_character(battle_character& c, const battle_character::route& r);
	void attack_character(battle_character& attacker,
	                      battle_character& defender,
						  const battle_move& move);
	bool can_make_move(const battle_character& c,
	                   const battle_move& move) const;

	const std::vector<battle_character_ptr>& participants() const { return chars_; }

	static int movement_duration();

	struct attack_stats {
		int attack, defense, damage, damage_critical, time_taken, energy_used;
	};

	attack_stats get_attack_stats(const battle_character& attacker,
	                       const battle_character& defender,
						   const battle_move& move,
						   std::string* description=NULL,
						   hex::location from_loc=hex::location()) const;
	
private:
	void remove_widget(gui::const_widget_ptr w);
	void draw(gui::slider* slider=NULL);
	void draw_route(const battle_character::route& r);
	hex::location selected_loc();
	battle_character_ptr selected_char();
	void handle_mouse_button_down(const SDL_MouseButtonEvent& e);
	const_battle_character_ptr is_engaged(
	       const battle_character& c) const;
	std::vector<battle_character_ptr> chars_;
	std::vector<battle_character_ptr>::iterator focus_;
	const hex::gamemap& map_;
	battle_character::move_map moves_;
	bool highlight_moves_;
	bool enter_attack_mode();
	bool enter_move_mode();
	std::set<hex::location> attacks_;
	bool highlight_attacks_;
	bool move_done_;
	bool turn_done_;
	hex::camera camera_;
	hex::camera_controller camera_controller_;

	void generate_movement_order();
	std::vector<graphics::texture> movement_order_;

	enum BATTLE_RESULT { ONGOING, PLAYER_WIN, PLAYER_LOSE };
	BATTLE_RESULT result_;

	std::vector<gui::widget_ptr> widgets_;

	gui::battle_menu_ptr menu_;

	const_battle_move_ptr current_move_;
	int keyed_selection_;
};
		
}

#endif
