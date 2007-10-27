
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
#include "battle_missile.hpp"
#include "battle_move.hpp"
#include "character.hpp"
#include "equipment.hpp"
#include "floating_label.hpp"
#include "formatter.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "frustum.hpp"
#include "initiative_bar.hpp"
#include "keyboard.hpp"
#include "label.hpp"
#include "model.hpp"
#include "preferences.hpp"
#include "raster.hpp"
#include "slider.hpp"
#include "status_bars_widget.hpp"
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
	 highlight_moves_(false), highlight_targets_(false),
	 move_done_(false), turn_done_(false),
	 camera_(battle_map), camera_controller_(camera_),
	 result_(ONGOING), keyed_selection_(0),
         current_time_(0), sub_time_(0.0),
         skippy_(50, preference_maxfps()),
         tracked_tile_(NULL),
		 initiative_bar_(new gui::initiative_bar)
{
	srand(SDL_GetTicks());

	for(std::vector<battle_character_ptr>::const_iterator i = chars_.begin();
	    i != chars_.end(); ++i) {
		gui::widget_ptr w(new game_dialogs::status_bars_widget(*this, *i));
		widgets_.push_back(w);

		initiative_bar_->add_character(*i);
	}
	time_cost_widget_.reset(new game_dialogs::time_cost_widget(*this));
	widgets_.push_back(time_cost_widget_);

	initiative_bar_->set_loc(graphics::screen_width() - 100, 50);
	initiative_bar_->set_dim(30, graphics::screen_height()/2);

	widgets_.push_back(initiative_bar_);

	std::sort(chars_.begin(), chars_.end(), battle_char_less);
}

void battle::play()
{
	while(result_ == ONGOING) {
		std::sort(chars_.begin(),chars_.end(),battle_char_less);
		focus_ = chars_.begin();

		// Update the initiative bar, moving the game time forward, until
		// someone is ready to move.
		while(current_time_ < (*focus_)->ready_to_move_at()) {
			sub_time_ = 0.0;
			while(sub_time_ < 1.0) {
				initiative_bar_->set_current_time(current_time_ + sub_time_);
				animation_frame(0.1);
			}

			++current_time_;
		}

		initiative_bar_->set_current_time(current_time_);
		sub_time_ = 0.0;
		(*focus_)->play_turn(*this);
	}

	if(result_ == QUIT) {
		SDL_Event e;
		e.type = SDL_QUIT;
		SDL_PushEvent(&e);
		return;
	}

	const std::string message = result_ == PLAYER_WIN ?
	     "Victory!" : "Defeat!";
	gui::widget_ptr msg = gui::label::create(message, result_ == PLAYER_WIN ? graphics::color_blue() : graphics::color_red(), 60);
	msg->set_loc((graphics::screen_width()-msg->width())/2,
	             (graphics::screen_height()-msg->height())/2);
	widgets_.push_back(msg);

	elapse_time(0.0, 100);

	graphics::floating_label::clear();
}

void battle::player_turn(battle_character& c)
{
	menu_.reset(new gui::battle_menu(*this,c));
	widgets_.push_back(menu_);
	move_done_ = false;
	turn_done_ = false;
	highlight_moves_ = false;
	highlight_targets_ = false;

	while(!turn_done_ && result_ == ONGOING) {
		if(!highlight_moves_ && !highlight_targets_) {
			const_battle_move_ptr move = menu_->highlighted_move();
			if(move) {
				std::cerr << "highlighted move: " << move->name() << ": " << move->min_moves() << "\n";
				initiative_bar_->focus_character(focus_->get(), move->min_moves() > 0 ? 0 : move->get_stat("initiative", **focus_));
			}
		}

		if(!skippy_.skip_frame()) {
			draw();
		}

		bool mouse_moved = false;
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			if(stats_dialogs_process_event(event)) {
				continue;
			}

			foreach(const gui::widget_ptr& w, widgets_) {
				w->process_event(event);
			}

			switch(event.type) {
				case SDL_QUIT:
					turn_done_ = true;
					result_ = QUIT;
					break;
				case SDL_KEYDOWN:
					if(highlight_targets_ &&
					   (event.key.keysym.sym == SDLK_RETURN ||
						event.key.keysym.sym == SDLK_SPACE)) {
						assert(current_move_);
						if(current_move_->can_attack()) {
							turn_done_ = true;
							battle_character_ptr target_char = selected_char();
							attack_character(**focus_, *target_char, *current_move_);
						} else {
							hex::location loc = selected_loc();
							if(map_.is_loc_on_map(loc)) {
								turn_done_ = true;
								assert(current_move_->mod());
								target_mod(**focus_, loc, *current_move_);
							}
						}
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
							highlight_targets_ = false;
						}
					} else if(highlight_targets_) {
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
			    case SDL_MOUSEMOTION:
					mouse_moved = true;
					break;
			}
		}

		if(mouse_moved) {
			handle_time_cost_popup();
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
					const battle_modification_ptr mod = current_move_->mod();
					if(mod && mod->target() != battle_modification::TARGET_SELF) {
						input = enter_target_mode();
					}
				}

				if(!input) {
					if(current_move_->mod()) {
						if(current_move_->mod()->target() == battle_modification::TARGET_SELF) {
							current_move_->mod()->apply(**focus_,**focus_,current_time_);
						}
					}

					(*focus_)->use_energy(current_move_->energy_required());
					(*focus_)->set_time_until_next_move(current_move_->get_stat("initiative",**focus_));
					initiative_bar_->focus_character(NULL);
					turn_done_ = true;
				}
			}
		}
	}

	highlight_moves_ = false;
	highlight_targets_ = false;
	targets_.clear();
	current_move_.reset();
	remove_widget(menu_);
	menu_.reset();
	initiative_bar_->focus_character(NULL);
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
	if(tracked_tile_) {
		tracked_tile_->clear_tracker();
	}

	using hex::tile;
	camera_controller_.prepare_selection();
	GLuint select_name = 0;
	foreach(const tile* t, tiles_) {
		glLoadName(select_name++);
		t->draw();
	}

	select_name = camera_controller_.finish_selection();
	if(select_name == GLuint(-1)) {
		return hex::location();
	}

	tracked_tile_ = tiles_[select_name];
	tracked_tile_->attach_tracker(&hex_tracker_);

	return tiles_[select_name]->loc();
}

battle_character_ptr battle::mouse_selected_char() {
	camera_controller_.prepare_selection();
	GLuint select_name = 0;
	foreach(const const_battle_character_ptr& c, chars_) {
		glLoadName(select_name++);
		c->draw();
	}

	select_name = camera_controller_.finish_selection();
	if(select_name == GLuint(-1)) {
		return battle_character_ptr();
	}
	return chars_[select_name];
}

battle_character_ptr battle::selected_char()
{
	using hex::tile;
	battle_character_ptr ret;

	ret = mouse_selected_char();
	if(ret) {
		return ret;
	}

	if(targets_.empty()) {
		return battle_character_ptr();
	}

	while(keyed_selection_ < 0) {
		keyed_selection_ += targets_.size();
	}

	keyed_selection_ = keyed_selection_%targets_.size();

	std::set<hex::location>::const_iterator i = targets_.begin();
	std::advance(i,keyed_selection_);
	foreach(const battle_character_ptr& c, chars_) {
		if(c->loc() == *i) {
			return c;
		}
	}

	return battle_character_ptr();
}

void battle::draw(gui::slider* slider)
{
	std::cerr << "drawing " << SDL_GetTicks() << "\n";
	using hex::tile;
	using hex::location;

	if(focus_ == chars_.end()) {
		focus_ = chars_.begin();
	}

	GLfloat pos[3];
	GLfloat rotate;
	(*focus_)->get_pos(pos, &rotate);
	camera_.set_pan(pos);

	battle_character::move_map::const_iterator selected_move = moves_.end();
	hex::location selected_hex;
	const_battle_character_ptr selected_character;
	if(highlight_moves_) {
		selected_move = moves_.find(selected_loc());
		if(selected_move != moves_.end()) {
			selected_hex = selected_move->first;
		}
	} else if(highlight_targets_) {
		selected_hex = selected_loc();
		if(targets_.count(selected_hex) == 0) {
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
	if(focus_ != chars_.end() && current_focus_ != (*focus_)->loc() || camera_.moved_since_last_check()) {
		rebuild_visible_tiles();
	}

	tile::setup_drawing();
	foreach(const tile* t, tiles_) {
		const bool dim = highlight_moves_ && moves_.count(t->loc()) == 0
		        || highlight_targets_ && targets_.count(t->loc()) == 0;
		if(dim) {
			glEnable(GL_LIGHT2);
			glDisable(GL_LIGHT0);
		}
		t->draw();
		t->draw_model();
		if(dim) {
			glEnable(GL_LIGHT0);
			glDisable(GL_LIGHT2);
		}
	}

	foreach(const tile* t, tiles_) {
		t->draw_cliffs();
	}

	hex::tile::draw_features(&tiles_[0], &tiles_[0] + tiles_.size(),
				 features_cache_);

	tile::finish_drawing();

	glDisable(GL_LIGHTING);

	//draw possible attacks
	if(highlight_moves_ && selected_hex.valid()) {
		foreach(const battle_character_ptr& c, chars_) {
			if((*focus_)->is_enemy(*c)) {
				(*focus_)->can_attack(*c, chars_, selected_hex, true);
			}
		}
	}

	glEnable(GL_LIGHTING);

	if(selected_move != moves_.end()) {
		draw_route(selected_move->second);
	}

	if(map_.is_loc_on_map(selected_hex)) {
		glDisable(GL_LIGHTING);
		int radius = 0;
		if(current_move_ && current_move_->mod()) {
			radius = current_move_->mod()->radius();
		}

		std::vector<hex::location> locs;
		hex::get_tiles_in_radius(selected_hex, radius, locs);
		foreach(const hex::location& loc, locs) {
			if(map_.is_loc_on_map(loc)) {
				map_.get_tile(loc).draw_highlight();
			}
		}
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

	if(missile_) {
		missile_->draw();
	}

	graphics::floating_label::update_labels();
	graphics::floating_label::draw_labels();

	particle_system_.draw();

	graphics::prepare_raster();

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

	if(highlight_targets_ && targets_.count(selected_hex)) {
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
			assert(current_move_);
			get_attack_stats(**focus_, *ch, *current_move_, &desc);
			graphics::texture text = graphics::font::render_text(desc, 20, color);
			graphics::blit_texture(text,50,50);
		}
	}

	foreach(const gui::widget_ptr& w, widgets_) {
		w->draw();
	}

	for(std::map<battle_character_ptr, game_dialogs::mini_stats_dialog_ptr>::iterator i =
		    stats_dialogs_.begin();
	    i != stats_dialogs_.end(); ++i) {
		i->second->draw();
	}

	std::cerr << "draw done...\n";

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

void battle::begin_animation() {
	stats_dialogs_.clear();
	time_cost_widget_->set_visible(false);
	time_cost_widget_->clear_tracker();
}

void battle::animation_frame(float t) {
	sub_time_ += t;

	if(!skippy_.skip_frame()) {
		draw();
	}
	/* do this to ensure key tables are updated */
	SDL_Event e;
	if(SDL_PollEvent(&e)) {
		SDL_PushEvent(&e);
	}
	camera_controller_.keyboard_control();
}

void battle::end_animation() {
	elapse_time(0.0, 25, true);
}

void battle::elapse_time(GLfloat anim_elapse, int frames, bool in_anim) {
	if(frames <= 0) {
		return;
	}
	const GLfloat step = anim_elapse/frames;
	if(!in_anim) {
		begin_animation();
	}
	for(int frame = 0; frame<frames;++frame) {
		animation_frame(step);
	}
	if(!in_anim) {
		end_animation();
	}
}

void battle::move_character(battle_character& c, const battle_character::route& r)
{
	begin_animation();

	const GLfloat time = c.begin_move(r);

	for(GLfloat t = 0; t < time; t += 0.1) {
		initiative_bar_->focus_character(&c, t);
		c.set_movement_time(t);
		animation_frame(0.1);
	}

	initiative_bar_->focus_character(&c, 0.0);

	c.end_move();
	end_animation();
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
	stats.attack = std::max<int>(ch.stat_mod_height(AttackStat,height_diff) +
	    move.get_stat(AttackStat,attacker), 1);
	stats.defense = std::max<int>(defender.defense(
	                  attacker.get_character().damage_type()), 0);
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
		case 2:
		case 3:
		case 4:
			stats.defense = defender.defense_behind();
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

	attacker.get_character().use_stamina(stats.stamina_used);
	attacker.begin_facing_change(
	  hex::get_main_direction(attacker.loc(),defender.loc()));

	if(!otherwise_engaged) {
		defender.begin_facing_change(
		  hex::get_main_direction(defender.loc(),attacker.loc()));
	}

	const int random = rand()%(stats.attack+stats.defense);

	GLfloat highlight[] = {1.0,0.0,0.0,0.5};
	const GLfloat elapsed_time = stats.time_taken;
	const GLfloat anim_time = elapsed_time/5.0;
	const GLfloat begin_hit = 0.4;
	const GLfloat end_hit = 0.8;
	attacker.begin_attack(defender);

	graphics::const_model_ptr missile;
	if(const equipment* weapon = attacker.get_character().weapon()) {
		const std::string& name = weapon->missile();
		if(!name.empty()) {
			graphics::const_model_ptr model = graphics::model::get_model(name);
			GLfloat src[3], dst[3];
			attacker.get_pos(src, NULL);
			defender.get_pos(dst, NULL);
			src[2] += 2.0;
			dst[2] += 2.0;
			missile_.reset(new battle_missile(model, src, dst));
		}
	}

	begin_animation();
	for(GLfloat t = 0.0; t < anim_time; t += 0.1) {
		std::cerr << "set init: " << (t*(elapsed_time/anim_time)) << "\n";
		initiative_bar_->focus_character(&attacker, t*(elapsed_time/anim_time));
		if(missile_.get()) {
			missile_->update();
		}
		const GLfloat cur_time = t / anim_time;
		attacker.set_movement_time(cur_time);
		defender.set_movement_time(cur_time);
		attacker.set_attack_time(cur_time);
		if(random > stats.defense &&
		   cur_time >= begin_hit && cur_time <= end_hit) {
			defender.set_highlight(highlight);
		}
		animation_frame(0.1 * elapsed_time/anim_time);
		defender.set_highlight(NULL);
	}
	end_animation();

	missile_.reset();
	attacker.end_attack();
	attacker.end_facing_change();
	defender.end_facing_change();

	std::cerr << "time until next: " << stats.time_taken << "\n";
	initiative_bar_->focus_character(&attacker, 0.0);
	attacker.set_time_until_next_move(stats.time_taken);
	(*focus_)->use_energy(attack_move.energy_required());

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

	defender.get_character().take_damage(damage);
	handle_dead_character(defender);
}

void battle::handle_dead_character(const battle_character& c)
{
	if(!c.get_character().dead()) {
		return;
	}

	elapse_time(0.0, 50);

	initiative_bar_->remove_character(&c);

	for(std::vector<battle_character_ptr>::iterator i = chars_.begin();
	    i != chars_.end(); ++i) {
		if(i->get() == &c) {
			chars_.erase(i);
			break;
		}
	}
	bool found_player = false;
	bool found_enemy = false;
	foreach(const battle_character_ptr& ch, chars_) {
		if(ch->is_human()) {
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

void battle::target_mod(battle_character& caster,
                        const hex::location& target,
                        const battle_move& move)
{
	const int time_to_perform = move.get_stat("initiative",caster);
	if(graphics::particle_emitter_ptr missile = move.create_missile_emitter()) {
		using hex::tile;
		const hex::location& src = caster.loc();
		assert(map_.is_loc_on_map(src));
		assert(map_.is_loc_on_map(target));
		const hex::tile& src_tile = map_.get_tile(src);
		const hex::tile& dst_tile = map_.get_tile(target);
		GLfloat src_pos[] = {tile::translate_x(src), tile::translate_y(src), tile::translate_height(src_tile.height())};
		GLfloat dst_pos[] = {tile::translate_x(target), tile::translate_y(target), tile::translate_height(dst_tile.height())};

		const GLfloat nframes = 100.0;

		begin_animation();
		for(GLfloat frame = 0.0; frame <= nframes; frame += 1.0) {
			GLfloat pos[3];
			for(int n = 0; n != 3; ++n) {
				pos[n] = dst_pos[n]*(frame/nframes) + src_pos[n]*((nframes-frame)/nframes);
			}

			missile->set_pos(pos);
			missile->emit_particle(particle_system_);
			animation_frame(time_to_perform/static_cast<GLfloat>(nframes));
		}
		end_animation();
	}

	assert(move.mod());
	const battle_modification& mod = *move.mod();
	caster.set_time_until_next_move(time_to_perform);
	caster.use_energy(move.energy_required());
	const battle_modification::TARGET_TYPE type = mod.target();
	const int radius = mod.radius();
	std::vector<hex::location> locs;
	std::vector<battle_character_ptr> affected_chars;
	get_tiles_in_radius(target, radius, locs);
	foreach(battle_character_ptr ch, chars_) {
		if(std::find(locs.begin(),locs.end(),ch->loc()) == locs.end()) {
			continue;
		}

		if(type == battle_modification::TARGET_ENEMY &&
		   !caster.is_enemy(*ch)) {
			continue;
		}

		if(type == battle_modification::TARGET_FRIEND &&
		   caster.is_enemy(*ch)) {
			continue;
		}
		affected_chars.push_back(ch);
	}
	foreach(battle_character_ptr ch, affected_chars) {
		mod.apply(caster, *ch, current_time_);
		handle_dead_character(*ch);
	}

}

bool battle::can_make_move(const battle_character& c,
                           const battle_move& move) const
{
	if(move.energy_required() > c.energy()) {
		std::cerr << "CANNOT MAKE MOVE: " << move.energy_required() << " > " << c.energy() << "\n";
		return false;
	}

	if(move.max_moves() > 0) {
		battle_character::move_map moves;
		c.get_possible_moves(moves, move, chars_);
		return !moves.empty();
	} else if(move.must_attack()) {
		foreach(const battle_character_ptr& enemy, chars_) {
			if(c.is_enemy(*enemy) && c.can_attack(*enemy, chars_)) {
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
		} else if(highlight_targets_ && current_move_->can_attack()) {
			battle_character_ptr target_char = selected_char();

			if(target_char && targets_.count(target_char->loc())) {
				turn_done_ = true;
				attack_character(**focus_, *target_char, *current_move_);
				if(move_done_) {
					turn_done_ = true;
				}

				highlight_targets_ = false;
				targets_.clear();
			}
		} else if(highlight_targets_) {
			hex::location loc = selected_loc();
			if(map_.is_loc_on_map(loc) && targets_.count(loc)) {
				turn_done_ = true;
				target_mod(**focus_, loc, *current_move_);
			}
		}
	}
}

bool battle::stats_dialogs_process_event(const SDL_Event& e) {
	const bool has_dialogs = !stats_dialogs_.empty();

	bool clear, grabbed;
	switch(e.type) {
	case SDL_KEYDOWN:
		if(has_dialogs) {
			clear = true;
			grabbed = true;
		} else {
			clear = false;
			grabbed = false;
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		if(e.button.button != SDL_BUTTON_RIGHT) {
			if(has_dialogs) {
				clear = true;
				grabbed = true;
			} else {
				clear = false;
				grabbed = false;
			}
			break;
		}
		{
			battle_character_ptr target_char = mouse_selected_char();
			if(target_char) {
				grabbed = true;
				clear = false;

				game_dialogs::mini_stats_dialog_ptr ptr;

				std::map<battle_character_ptr,game_dialogs::mini_stats_dialog_ptr>::iterator i =
					stats_dialogs_.find(target_char);
				if(i == stats_dialogs_.end()) {
					ptr.reset(new game_dialogs::mini_stats_dialog(target_char, 200, 100));
					stats_dialogs_[target_char] = ptr;
					ptr->show();
					ptr->set_frame(gui::frame_manager::make_frame(ptr, "mini-char-battle-stats"));
				} else {
					stats_dialogs_.erase(i);
				}
			} else {
				if(has_dialogs) {
					grabbed = true;
					clear = true;
				} else {
					grabbed = false;
					clear = false;
				}
			}
		}
		break;
	default:
		clear = false;
		grabbed = false;
		break;
	}

	if(clear) {
		stats_dialogs_.clear();
	}

	return grabbed;
}

void battle::handle_time_cost_popup()
{
	battle_character_ptr attacker = *focus_;

	if(highlight_moves_ ) {
		const battle_character::move_map::const_iterator move = moves_.find(selected_loc());
		if(move != moves_.end()) {
			int cost = attacker->route_cost(move->second);
			initiative_bar_->focus_character(attacker.get(), cost);
			time_cost_widget_->set_tracker(&hex_tracker_);
			time_cost_widget_->set_time_cost(cost);
			time_cost_widget_->set_visible(true);
			return;
		} else {
			initiative_bar_->focus_character(attacker.get(), 0);
		}
	} else if(highlight_targets_) {
		battle_character_ptr defender = selected_char();
		assert(current_move_);
		if(current_move_->can_attack()) {
			/* attack cost */
			attack_stats stats = get_attack_stats(*attacker, *defender, *current_move_);
			time_cost_widget_->set_tracker(&(defender->loc_tracker()));
			time_cost_widget_->set_time_cost(stats.time_taken);
			initiative_bar_->focus_character(attacker.get(), stats.time_taken);
			time_cost_widget_->set_visible(true);
			return;
		} else {
			/* SPELL cost */
			hex::location loc = selected_loc();
			if(map_.is_loc_on_map(loc)) {
				/* move cost */
				time_cost_widget_->set_tracker(&hex_tracker_);
				const int cost = current_move_->get_stat("initiative",*attacker);
				time_cost_widget_->set_time_cost(cost);
				initiative_bar_->focus_character(attacker.get(), cost);
				time_cost_widget_->set_visible(true);
				return;
			}
		}
	}

	time_cost_widget_->clear_tracker();
	time_cost_widget_->set_visible(false);
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
	targets_.clear();

	if(current_move_ && current_move_->can_attack() == false) {
		return false;
	}

	foreach(const battle_character_ptr& c, chars_) {
		if(*focus_ != c && (*focus_)->is_enemy(*c) && (*focus_)->can_attack(*c, chars_)) {
			targets_.insert(c->loc());
		}
	}
	highlight_targets_ = targets_.empty() == false;
	return highlight_targets_;
}

bool battle::enter_target_mode()
{
	highlight_moves_ = false;
	targets_.clear();

	battle_modification_ptr mod = current_move_->mod();
	if(!mod) {
		return false;
	}

	std::vector<hex::location> locs;
	hex::get_tiles_in_radius((*focus_)->loc(), mod->range(), locs);
	foreach(const hex::location& loc, locs) {
		targets_.insert(loc);
	}
	highlight_targets_ = targets_.empty() == false;
	return highlight_targets_;
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

namespace {
hex::frustum view_volume;
}

void battle::rebuild_visible_tiles()
{
	std::cerr << "rebuild visible tiles\n";
	tiles_.clear();
	hex::frustum::initialize();
	view_volume.set_volume_clip_space(-1, 1, -1, 1, -1, 1);
	const hex::location& loc = (*focus_)->loc();
	current_focus_ = loc;

	hex::location hex_dir[6];
	hex::get_adjacent_tiles(loc, hex_dir);
	int core_radius = 1;
	bool done = false;
	while(!done) {
		for(int n = 0; n != 6; ++n) {
			hex_dir[n] = hex::tile_in_direction(hex_dir[n], static_cast<hex::DIRECTION>(n));
			if(!map_.is_loc_on_map(hex_dir[n])) {
				done = true;
				break;
			}

			const hex::tile& t = map_.get_tile(hex_dir[n]);
			if(!view_volume.intersects(t)) {
				done = true;
				break;
			}
		}

		++core_radius;
	}

	done = false;
	std::vector<hex::location> hexes;
	for(int radius = 0; !done; ++radius) {
		hexes.clear();
		hex::get_tile_ring(loc, radius, hexes);
		done = true;
		foreach(const hex::location& location, hexes) {
			if(!map_.is_loc_on_map(location)) {
				continue;
			}

			const hex::tile& t = map_.get_tile(location);
			if(radius >= core_radius && !view_volume.intersects(t)) {
				//see if this tile has a cliff which is visible, in which case ew should draw it
				const hex::tile* cliffs[6];
				const int num_cliffs = t.neighbour_cliffs(cliffs);
				bool found = false;
				for(int n = 0; n != num_cliffs; ++n) {
					if(view_volume.intersects(*cliffs[n])) {
						found = true;
						break;
					}
				}

				if(!found) {
					continue;
				}
			} else {
				done = false;
			}

			tiles_.push_back(&t);
			tiles_.back()->load_texture();
		}
	}

	std::sort(tiles_.begin(), tiles_.end(), hex::tile::compare_texture());

	hex::tile::initialize_features_cache(
		&tiles_[0], &tiles_[0] + tiles_.size(), &features_cache_);
}

}
