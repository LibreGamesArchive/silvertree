#ifndef STATUS_BARS_WIDGET_HPP_INCLUDED
#define STATUS_BARS_WIDGET_HPP_INCLUDED

#include <math.h>

#include "battle.hpp"
#include "battle_character.hpp"
#include "widget.hpp"

namespace game_dialogs {

class status_bars_widget: public gui::widget {
public:
	status_bars_widget(const game_logic::battle& b, const game_logic::battle_character_ptr ch)
		: hitpoints_(-1), old_hitpoints_(-1), max_hitpoints_(-1), old_max_hitpoints_(-1),
		  cur_move_time_(-HUGE_VALF), prev_move_time_(-HUGE_VALF), ch_(ch), b_(b) {}
private:
	void handle_draw() const;
	mutable GLfloat hitpoints_, old_hitpoints_;
	mutable GLfloat max_hitpoints_, old_max_hitpoints_;
	mutable GLfloat cur_move_time_, prev_move_time_;

	const game_logic::battle_character_ptr ch_;
	const game_logic::battle& b_;
};

}

#endif
