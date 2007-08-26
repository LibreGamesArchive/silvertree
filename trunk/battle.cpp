
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "battle.hpp"
#include "battle_menu.hpp"
#include "battle_move.hpp"
#include "character.hpp"
#include "floating_label.hpp"
#include "formatter.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "keyboard.hpp"
#include "label.hpp"
#include "raster.hpp"
#include "slider.hpp"
#include "surface_cache.hpp"

#include <boost/lexical_cast.hpp>
#include <iostream>
#include <sstream>

namespace game_logic
{

namespace {

bool battle_char_less(const const_battle_character_ptr& c1,
                      const const_battle_character_ptr& c2)
{
	return c1->ready_to_move_at() < c2->ready_to_move_at();
}
		
}

battle::battle(const std::vector<battle_character_ptr>& chars,
               const hex::gamemap& battle_map)
   : chars_(chars), focus_(chars_.end()), map_(battle_map),
	 highlight_moves_(false), highlight_attacks_(false),
	 move_done_(false), turn_done_(false),
	 camera_(battle_map), camera_controller_(camera_),
	 result_(ONGOING), keyed_selection_(0),
	 current_time_(0)
{
	srand(SDL_GetTicks());
}

void battle::play()
{
	while(result_ == ONGOING) {
		std::sort(chars_.begin(),chars_.end(),battle_char_less);
		generate_movement_order();
		focus_ = chars_.begin();
		current_time_ = (*focus_)->ready_to_move_at();
		(*focus_)->play_turn(*this);
	}

	const std::string message = result_ == PLAYER_WIN ?
	     "Victory!" : "Defeat!";
	gui::widget_ptr msg = gui::label::create(message, result_ == PLAYER_WIN ? graphics::color_blue() : graphics::color_red(), 60);
	msg->set_loc((graphics::screen_width()-msg->width())/2,
	             (graphics::screen_height()-msg->height())/2);
	widgets_.push_back(msg);

	const int show_until = SDL_GetTicks() + 2000;
	while(SDL_GetTicks() < show_until) {
		draw();
		SDL_Delay(10);
	}

	graphics::floating_label::clear();
}

void battle::player_turn(battle_character& c)
{
	menu_.reset(new gui::battle_menu(*this,c));
	widgets_.push_back(menu_);
	move_done_ = false;
	turn_done_ = false;
	highlight_moves_ = false;
	highlight_attacks_ = false;
	while(!turn_done_ && result_ == ONGOING) {
		draw();

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			foreach(const gui::widget_ptr& w, widgets_) {
				w->process_event(event);
			}

			switch(event.type) {
				case SDL_QUIT:
					turn_done_ = true;
					break;
				case SDL_KEYDOWN:
					if(highlight_attacks_ &&
					   (event.key.keysym.sym == SDLK_RETURN ||
						event.key.keysym.sym == SDLK_SPACE)) {
						battle_character_ptr target_char = selected_char();
						turn_done_ = true;
						attack_character(**focus_, *target_char, *current_move_);
					} else if(current_move_ &&
					   (event.key.keysym.sym == SDLK_ESCAPE ||
						event.key.keysym.sym == SDLK_RETURN ||
						event.key.keysym.sym == SDLK_SPACE)) {

						if(move_done_) {
							turn_done_ = true;
						} else {
							current_move_.reset();
							menu_.reset(new gui::battle_menu(*this,c));
							widgets_.push_back(menu_);
						}
					} else if(highlight_attacks_) {
						switch(event.key.keysym.sym) {
							case SDLK_RIGHT:
								++keyed_selection_;
								break;
							case SDLK_LEFT:
								--keyed_selection_;
								break;
						}
					}
					break;
				case SDL_MOUSEBUTTONDOWN:
					handle_mouse_button_down(event.button);
					break;
			}
		}

		camera_controller_.keyboard_control();

		if(!current_move_) {
			current_move_ = menu_->selected_move();
			if(current_move_) {
				remove_widget(menu_);
				menu_.reset();
				bool input = false;
				if(current_move_->max_moves() > 0) {
					input = enter_move_mode();
				}

				if(!input && current_move_->can_attack()) {
					input = enter_attack_mode();
				}

				if(!input) {
					if(current_move_->mod()) {
						if(current_move_->mod()->target() == battle_modification::TARGET_SELF) {
							current_move_->mod()->apply(**focus_,**focus_,current_time_);
						}
					}

					(*focus_)->set_time_until_next_move(current_move_->get_stat("initiative",**focus_));
					turn_done_ = true;
				}
			}
		}
	}

	highlight_moves_ = false;
	highlight_attacks_ = false;
	attacks_.clear();
	current_move_.reset();
	remove_widget(menu_);
	menu_.reset();
}

void battle::remove_widget(gui::const_widget_ptr w)
{
	widgets_.erase(std::remove(widgets_.begin(),widgets_.end(),w),widgets_.end());
}

int battle::movement_duration()
{
	return 10;
}

hex::location battle::selected_loc()
{
	using hex::tile;
	camera_controller_.prepare_selection();
	const std::vector<tile>& tiles = map_.tiles();
	GLuint select_name = 0;
	foreach(const tile& t, tiles) {
		glLoadName(select_name++);
		t.draw();
	}

	select_name = camera_controller_.finish_selection();
	if(select_name == GLuint(-1)) {
		return hex::location();
	}

	return tiles[select_name].loc();
}

battle_character_ptr battle::selected_char()
{
	using hex::tile;
	camera_controller_.prepare_selection();
	GLuint select_name = 0;
	foreach(const const_battle_character_ptr& c, chars_) {
		glLoadName(select_name++);
		c->draw();
	}

	select_name = camera_controller_.finish_selection();
	if(select_name == GLuint(-1)) {
		if(attacks_.empty()) {
			return battle_character_ptr();
		}

		while(keyed_selection_ < 0) {
			keyed_selection_ += attacks_.size();
		}

		keyed_selection_ = keyed_selection_%attacks_.size();

		std::set<hex::location>::const_iterator i = attacks_.begin();
		std::advance(i,keyed_selection_);
		foreach(const battle_character_ptr& c, chars_) {
			if(c->loc() == *i) {
				return c;
			}
		}

		return battle_character_ptr();
	} else {
		return chars_[select_name];
	}
}

void battle::draw(gui::slider* slider)
{
	using hex::tile;
	using hex::location;

	if(focus_ != chars_.end()) {
		GLfloat pos[3];
		GLfloat rotate;
		(*focus_)->get_pos(pos, &rotate);
		camera_.set_pan(pos);
	}

	battle_character::move_map::const_iterator selected_move = moves_.end();
	hex::location selected_hex;
	const_battle_character_ptr selected_character;
	if(highlight_moves_) {
		selected_move = moves_.find(selected_loc());
		if(selected_move != moves_.end()) {
			selected_hex = selected_move->first;
		}
	} else if(highlight_attacks_) {
		selected_hex = selected_loc();
		if(attacks_.count(selected_hex) == 0) {
			selected_character = selected_char();
			if(selected_character) {
				selected_hex = selected_character->loc();
				selected_character = const_battle_character_ptr();
			} else {
				selected_hex = hex::location();
			}
		}
	} else {
		selected_character = selected_char();
	}

	camera_.prepare_frame();

	tile::setup_drawing();
	const std::vector<tile>& tiles = map_.tiles();
	foreach(const tile& t, tiles) {
		const bool dim = highlight_moves_ && moves_.count(t.loc()) == 0
		        || highlight_attacks_ && attacks_.count(t.loc()) == 0;
		if(dim) {
			glEnable(GL_LIGHT2);
			glDisable(GL_LIGHT0);
		}
		t.draw();
		t.draw_model();
		if(dim) {
			glEnable(GL_LIGHT0);
			glDisable(GL_LIGHT2);
		}
	}

	tile::finish_drawing();

	foreach(const tile& t, tiles) {
		t.draw_cliffs();
	}


	glDisable(GL_LIGHTING);
	map_.draw_grid();

	//draw possible attacks
	if(highlight_moves_ && selected_hex.valid()) {
		foreach(const battle_character_ptr& c, chars_) {
			if((*focus_)->is_enemy(*c)) {
				(*focus_)->can_attack(*c, selected_hex, true);
			}
		}
	}

	glEnable(GL_LIGHTING);

	if(selected_move != moves_.end()) {
		draw_route(selected_move->second);
	}

	if(map_.is_loc_on_map(selected_hex)) {
		glDisable(GL_LIGHTING);
		map_.get_tile(selected_hex).draw_highlight();
		glEnable(GL_LIGHTING);
	}

	if(map_.is_loc_on_map((*focus_)->loc())) {
		glDisable(GL_LIGHTING);
		map_.get_tile((*focus_)->loc()).draw_highlight();
		glEnable(GL_LIGHTING);
	}

	if(focus_ != chars_.end()) {
		draw_route((*focus_)->movement_plan());
	}

	foreach(const_battle_character_ptr c, chars_) {
		c->draw();
	}

	graphics::floating_label::update_labels();
	graphics::floating_label::draw_labels();

	graphics::prepare_raster();

	int order_x = 800;
	int order_y = 50;
	foreach(const graphics::texture& text, movement_order_) {
		graphics::blit_texture(text,order_x,order_y);
		order_y += 20;
	}
	
	if(slider) {
		slider->draw();
	}

	if(selected_character) {
		SDL_Color color = {0xFF,0xFF,0xFF,0xFF};
		std::vector<graphics::texture> text;
		graphics::font::render_multiline_text(selected_character->status_text(), 20, color, text);
		int x = 50;
		int y = 50;
		foreach(graphics::texture& t, text) {
			graphics::blit_texture(t,x,y);
			y += t.height();
		}
	}

	if(highlight_attacks_ && attacks_.count(selected_hex)) {
		battle_character_ptr ch;
		foreach(const battle_character_ptr& c, chars_) {
			if(c->loc() == selected_hex) {
				ch = c;
				break;
			}
		}

		if(ch) {
			SDL_Color color = {0xFF,0xFF,0xFF,0xFF};
			std::string desc;
			get_attack_stats(**focus_, *ch, *current_move_, &desc);
			graphics::texture text = graphics::font::render_text(desc, 20, color);
			graphics::blit_texture(text,50,50);
		}
	}

	foreach(const gui::widget_ptr& w, widgets_) {
		w->draw();
	}

	SDL_GL_SwapBuffers();
	SDL_Delay(1);
}

void battle::draw_route(const battle_character::route& r)
{
	if(r.size() <= 1) {
		return;
	}
	
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glBegin(GL_LINE_STRIP);
	glColor3f(1.0,1.0,1.0);
	foreach(const hex::location& loc, r) {
		assert(map_.is_loc_on_map(loc));
		map_.get_tile(loc).draw_center();
	}

	glEnd();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
}

void battle::move_character(battle_character& c, const battle_character::route& r)
{
	const GLfloat time = c.begin_move(r);
	for(GLfloat t = 0.0; t < time; t += 0.1) {
		c.set_movement_time(t);
		draw();
	}

	c.end_move();
}

battle::attack_stats battle::get_attack_stats(
     const battle_character& attacker,
	 const battle_character& defender,
	 const battle_move& move,
	 std::string* description,
	 hex::location from_loc) const
{
	if(!from_loc.valid()) {
		from_loc = attacker.loc();
	}

	const int height_diff = map_.get_tile(from_loc).height() -
	                        map_.get_tile(defender.loc()).height();
	std::cerr << "height diff: " << height_diff << "\n";
	const character& ch = attacker.get_character();
	static const std::string AttackStat = "attack";
	static const std::string DefenseStat = "defense";
	static const std::string DamageStat = "damage";
	static const std::string InitiativeStat = "initiative";
	static const std::string StaminaUsedStat = "stamina_used";
	attack_stats stats;
	stats.attack = ch.stat_mod_height(AttackStat,height_diff) +
	    move.get_stat(AttackStat,attacker);
	stats.defense = defender.defense(
	                  attacker.get_character().damage_type());
	stats.damage = ch.stat_mod_height(DamageStat,height_diff) +
	                  move.get_stat(DamageStat,attacker);
	stats.damage_critical = stats.damage;
	stats.time_taken = ch.stat_mod_height(InitiativeStat,height_diff) +
	                       move.get_stat(InitiativeStat,attacker);
	stats.stamina_used = ch.stat_mod_height(StaminaUsedStat,height_diff) +
	                       move.get_stat(StaminaUsedStat,attacker);
	int resist_amount, resist_percent;
	defender.get_character().get_resistance(
	                        attacker.get_character().damage_type(),
	                        &resist_amount, &resist_percent);
	stats.damage -=
	    (std::min(resist_amount,stats.damage)*resist_percent)/100;

	bool behind = false;
	const bool engaged = is_engaged(defender);
	if(engaged) {
		switch(abs(hex::get_adjacent_direction(defender.loc(),
		                 from_loc) - defender.facing())) {
		case 3:
			stats.defense /= 3;
			behind = true;
			break;
		case 2:
		case 4:
			stats.defense /= 2;
			behind = true;
			break;
		}
	}

	if(description) {
		const int chance_to_hit = (100*stats.attack)/(stats.defense+stats.attack);
		*description += formatter() << "Attack: " <<
		                stats.attack << "\nDefense: " <<
						stats.defense <<
						(behind ? " (from behind!)" : "") << "\nChance to hit: " << chance_to_hit <<
						"%\nDamage: " << stats.damage <<
						"\nTime: " << stats.time_taken << "s";
		if(stats.attack > stats.defense &&
		   stats.damage_critical != stats.damage) {
			const int chance_to_critical = (100*(stats.attack-stats.defense)/(stats.defense+stats.attack));
			*description += formatter() << "Chance to critical: " <<
			                chance_to_critical <<
							"%\nCritical damage: " <<
							stats.damage_critical;
		}
						
	}

	return stats;
}

void battle::attack_character(battle_character& attacker,
                              battle_character& defender,
							  const battle_move& attack_move)
{
	attack_stats stats = get_attack_stats(attacker,defender,attack_move);
	const_battle_character_ptr engaged_with = is_engaged(defender);
	const bool otherwise_engaged = engaged_with &&
	                 engaged_with->loc() != attacker.loc();

	attacker.set_time_until_next_move(stats.time_taken);
	attacker.get_character().use_stamina(stats.stamina_used);
	attacker.begin_facing_change(
	  hex::get_adjacent_direction(attacker.loc(),defender.loc()));

	if(!otherwise_engaged) {
		defender.begin_facing_change(
		  hex::get_adjacent_direction(defender.loc(),attacker.loc()));
	}

	const int random = rand()%(stats.attack+stats.defense);

	GLfloat highlight[] = {1.0,0.0,0.0,0.5};
	const GLfloat time = 1.0;
	const GLfloat begin_hit = 0.4;
	const GLfloat end_hit = 0.4;
	attacker.begin_attack(defender);
	const int ticks = int(time*1000.0);
	const int beg = SDL_GetTicks();
	const int end = beg + ticks;
	int cur_ticks;
	while((cur_ticks = SDL_GetTicks()) < end) {
		const GLfloat cur_time = GLfloat(cur_ticks - beg)/1000.0;
		attacker.set_movement_time(cur_time);
		defender.set_movement_time(cur_time);
		attacker.set_attack_time(cur_time);
		if(random > stats.defense &&
		   cur_time >= begin_hit && cur_time <= end_hit) {
			defender.set_highlight(highlight);
		}
		draw();
		defender.set_highlight(NULL);
	}
	attacker.end_attack();
	attacker.end_facing_change();
	defender.end_facing_change();

	int damage = stats.damage;
	if(random <= stats.defense) {
		damage = 0;
	} else if(random >= stats.defense*2) {
		damage = stats.damage_critical;
	}

	std::cerr << "rand: " << random << "/" << stats.defense << " -> " << damage << "\n";

	const SDL_Color red = {0xFF,0x0,0x0,0xFF};
	const SDL_Color blue = {0x0,0x0,0xFF,0xFF};
	GLfloat pos[3];
	const GLfloat move[3] = {0.0,0.0,0.01};
	GLfloat rotate;
	defender.get_pos(pos,&rotate);
	if(damage) {
		graphics::texture damage_tex = graphics::font::render_text(boost::lexical_cast<std::string>(damage), 20, red);
		graphics::floating_label::add(damage_tex,pos,move,1000);
	} else {
		graphics::texture damage_tex = graphics::font::render_text("miss!", 20, blue);
		graphics::floating_label::add(damage_tex,pos,move,1000);
	}

	const bool dead = defender.get_character().take_damage(damage);

	if(dead) {
		for(std::vector<battle_character_ptr>::iterator i = chars_.begin(); i != chars_.end(); ++i) {
			if(i->get() == &defender) {
				chars_.erase(i);
				break;
			}
		}
		bool found_player = false;
		bool found_enemy = false;
		foreach(const battle_character_ptr& c, chars_) {
			if(c->is_human()) {
				found_player = true;
			} else {
				found_enemy = true;
			}
		}

		if(found_player && !found_enemy) {
			result_ = PLAYER_WIN;
		} else if(found_enemy && !found_player) {
			result_ = PLAYER_LOSE;
		}
	}
}

bool battle::can_make_move(const battle_character& c,
                           const battle_move& move) const
{
	if(move.max_moves() > 0) {
		battle_character::move_map moves;
		c.get_possible_moves(moves, move, chars_);
		return !moves.empty();
	} else if(move.must_attack()) {
		foreach(const battle_character_ptr& enemy, chars_) {
			if(c.is_enemy(*enemy) && c.can_attack(*enemy)) {
				return true;
			}
		}

		return false;
	}

	return true;
}

void battle::handle_mouse_button_down(const SDL_MouseButtonEvent& e)
{
	if(e.button == SDL_BUTTON_LEFT) {
		if(highlight_moves_) {
			const battle_character::move_map::const_iterator move = moves_.find(selected_loc());
			if(move != moves_.end()) {
				move_character(**focus_, move->second);
				turn_done_ = !enter_attack_mode();
				move_done_ = true;
			}
		} else if(highlight_attacks_) {
			battle_character_ptr target_char = selected_char();

			if(target_char) {
				turn_done_ = true;
				attack_character(**focus_, *target_char, *current_move_);
			}

			if(move_done_) {
				turn_done_ = true;
			}

			highlight_attacks_ = false;
			attacks_.clear();
		}
	}
}

bool battle::enter_move_mode()
{
	if(move_done_) {
		return false;
	}
	(*focus_)->get_possible_moves(moves_, *current_move_, chars_);
	highlight_moves_ = true;
	return true;
}

bool battle::enter_attack_mode()
{
	highlight_moves_ = false;
	attacks_.clear();

	if(current_move_ && current_move_->can_attack() == false) {
		return false;
	}

	foreach(const battle_character_ptr& c, chars_) {
		if(*focus_ != c && (*focus_)->is_enemy(*c) && (*focus_)->can_attack(*c)) {
			attacks_.insert(c->loc());
		}
	}
	highlight_attacks_ = attacks_.empty() == false;
	return highlight_attacks_;
}

void battle::generate_movement_order()
{
	const SDL_Color red = {0xFF,0x00,0x00,0xFF};
	const SDL_Color green = {0x00,0xFF,0x00,0xFF};
	movement_order_.clear();
	const int now = chars_.front()->ready_to_move_at();
	foreach(const battle_character_ptr& c, chars_) {
		std::ostringstream str;
		str << c->get_character().description() << ": "
		    << (c->ready_to_move_at()-now);
		movement_order_.push_back(graphics::font::render_text(str.str(),20,c->is_human_controlled() ? green : red));
	}
}

const_battle_character_ptr battle::is_engaged(
      const battle_character& c) const
{
	using hex::location;
	const location loc = hex::tile_in_direction(c.loc(),c.facing());
	foreach(const const_battle_character_ptr& a, chars_) {
		if(a->loc() == loc) {
			if(hex::tile_in_direction(a->loc(),a->facing()) == c.loc()){
				return a;
			} else {
				return const_battle_character_ptr();
			}
		}
	}

	return const_battle_character_ptr();
}

}
