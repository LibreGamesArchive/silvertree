
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

#include <boost/scoped_ptr.hpp>
#include <set>
#include <vector>

#include "battle_character.hpp"
#include "battle_menu_fwd.hpp"
#include "battle_missile.hpp"
#include "battle_move_fwd.hpp"
#include "camera.hpp"
#include "camera_controller.hpp"
#include "frame_rate_utils.hpp"
#include "gamemap.hpp"
#include "initiative_bar_fwd.hpp"
#include "location_tracker.hpp"
#include "mini_stats_dialog.hpp"
#include "particle_system.hpp"
#include "time_cost_widget.hpp"
#include "texture.hpp"
#include "widget.hpp"

namespace gui
{
class slider;
}

namespace game_logic
{

class battle_modification;

class battle
{
public:
	battle(const std::vector<battle_character_ptr>& chars,
	       const hex::gamemap& battle_map);

	void play();
	void player_turn(battle_character& c);
	const hex::gamemap& map() const { return map_; }

	void elapse_time(GLfloat anim_elapse, int frames, bool in_anim = false);
	void move_character(battle_character& c, const battle_character::route& r);
	void attack_character(battle_character& attacker,
	                      battle_character& defender,
			      const battle_move& move);
	void target_mod(battle_character& ch,
	                const hex::location& target,
	                const battle_move& move);
	bool can_make_move(const battle_character& c,
	                   const battle_move& move) const;

	const std::vector<battle_character_ptr>& participants() const { return chars_; }

	static int movement_duration();

	struct attack_stats {
		int attack, defense, damage, damage_critical, time_taken, stamina_used;
	};

	attack_stats get_attack_stats(const battle_character& attacker,
	                       const battle_character& defender,
						   const battle_move& move,
						   std::string* description=NULL,
						   hex::location from_loc=hex::location()) const;
	int current_time() const { return current_time_; }
	GLfloat animation_time() const { return sub_time_; }
	const battle_character_ptr active_character() const { return *focus_; }

	hex::camera& camera() { return camera_; }
	const hex::camera& camera() const { return camera_; }

	void draw(gui::slider* slider=NULL, bool draw_widgets=true);
private:
	void remove_widget(gui::const_widget_ptr w);
	void draw_route(const battle_character::route& r);
	hex::location selected_loc();
	battle_character_ptr selected_char();
	battle_character_ptr mouse_selected_char();
	void handle_mouse_button_down(const SDL_MouseButtonEvent& e);
	bool stats_dialogs_process_event(const SDL_Event& e);
	void handle_time_cost_popup();
	void begin_animation();
	void end_animation();
	void animation_frame(GLfloat t);
	const_battle_character_ptr is_engaged(
	       const battle_character& c) const;
	void handle_dead_character(const battle_character& c);
	std::vector<battle_character_ptr> chars_;
	std::vector<battle_character_ptr>::iterator focus_;
	const hex::gamemap& map_;

	void rebuild_visible_tiles();
	std::vector<const hex::tile*> tiles_;
	hex::location current_focus_;
	battle_character::move_map moves_;
	hex::tile::features_cache features_cache_;
	bool highlight_moves_;
	bool enter_attack_mode();
	bool enter_target_mode();
	bool enter_move_mode();
	std::set<hex::location> targets_;
	bool highlight_targets_;
	bool move_done_;
	bool turn_done_;
	hex::camera camera_;
	hex::camera_controller camera_controller_;

	enum BATTLE_RESULT { ONGOING, PLAYER_WIN, PLAYER_LOSE, QUIT };
	BATTLE_RESULT result_;

	std::vector<gui::widget_ptr> widgets_;
	std::map<battle_character_ptr, game_dialogs::mini_stats_dialog_ptr> stats_dialogs_;

	gui::battle_menu_ptr menu_;

	const_battle_move_ptr current_move_;
	int keyed_selection_;
	int current_time_;
	GLfloat sub_time_;

	graphics::frame_skipper skippy_;
	graphics::location_tracker hex_tracker_;
	const hex::tile* tracked_tile_;
	game_dialogs::time_cost_widget_ptr time_cost_widget_;
	graphics::particle_system particle_system_;
	boost::scoped_ptr<battle_missile> missile_;

	gui::initiative_bar_ptr initiative_bar_;
};

}

#endif
