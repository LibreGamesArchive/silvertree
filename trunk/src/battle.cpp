
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
#include "world.hpp"

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
      tracked_tile_(NULL), 
      initiative_bar_(new gui::initiative_bar),
      listener_(this),
      renderer_(map_, camera_),
      selection_(renderer_)
{
    srand(SDL_GetTicks());
    
    for(std::vector<battle_character_ptr>::const_iterator i = chars_.begin();
        i != chars_.end(); ++i) {

        gui::widget_ptr w(new game_dialogs::status_bars_widget(*this, *i));
        add_widget(w);
        
        initiative_bar_->add_character(*i);
    }
    time_cost_widget_.reset(new game_dialogs::time_cost_widget(*this));
    add_widget(time_cost_widget_);
    
    initiative_bar_->set_loc(graphics::screen_width() - 100, 50);
    initiative_bar_->set_dim(30, graphics::screen_height()/2);
    
    add_widget(initiative_bar_);
    
    std::sort(chars_.begin(), chars_.end(), battle_char_less);

    pump_.register_listener(&listener_);
    pump_.register_listener(&camera_controller_);
    pump_.register_listener(&selection_);
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
                animation_frame(0.5);
				pump_.process();
            }
            // Check whether character modifications expired.
            foreach(battle_character_ptr ch, chars_) {
                ch->update_time(current_time_);
            }
            
            ++current_time_;
        }
        
        initiative_bar_->set_current_time(current_time_);
        sub_time_ = 0.0;
        (*focus_)->play_turn(*this);
    }

    if(result_ == QUIT) {
        graphics::floating_label::clear();
        return;
    }
    const std::string message = result_ == PLAYER_WIN ?
        "Victory!" : "Defeat!";
    gui::widget_ptr msg = gui::label::create(message, result_ == PLAYER_WIN ? graphics::color_blue() : graphics::color_red(), 60);
    msg->set_loc((graphics::screen_width()-msg->width())/2,
                 (graphics::screen_height()-msg->height())/2);
    add_widget(msg);
    
    elapse_time(0.0, 100);
    
    graphics::floating_label::clear();
}

void battle::player_turn(battle_character& c)
{
    listener_.set_character(c);
    menu_.reset(new gui::battle_menu(*this,c));
    add_widget(menu_);
    move_done_ = false;
    turn_done_ = false;
    highlight_moves_ = false;
    highlight_targets_ = false;
    
    while(!turn_done_ && result_ == ONGOING) {
        if(!highlight_moves_ && !highlight_targets_) {
            const_battle_move_ptr move = menu_->highlighted_move();
            if(move) {
                //std::cerr << "highlighted move: " << move->name() << ": " << move->min_moves() << "\n";
                initiative_bar_->focus_character(focus_->get(), move->min_moves() > 0 ? 0 : move->get_stat("initiative", **focus_));
            }
        }
        
        if(draw()) {
            draw_widgets(NULL);
        }
        SDL_GL_SwapBuffers();
        
        if(!pump_.process()) {
            turn_done_ = true;
            result_ = QUIT;
        }
        
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
        camera_controller_.update();
    }
    
    highlight_moves_ = false;
    highlight_targets_ = false;
    targets_.clear();
    current_move_.reset();
    remove_widget(menu_);
    menu_.reset();
    initiative_bar_->focus_character(NULL);
}

void battle::add_widget(gui::widget_ptr w) 
{
    widgets_.push_back(w);
    pump_.register_listener(w);
}

void battle::remove_widget(gui::widget_ptr w)
{
    widgets_.erase(std::remove(widgets_.begin(),widgets_.end(),w),widgets_.end());
    pump_.deregister_listener(w);
}

int battle::movement_duration()
{
	return 10;
}

battle_character_ptr battle::mouse_selected_char() {
    int select_name = selection_.get_selected_avatar();
    
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


bool battle::draw()
{
    using hex::tile;
    using hex::location;

    graphics::floating_label::update_labels();
    
    if(focus_ == chars_.end()) {
        focus_ = chars_.begin();
    }
    
    renderer_.reset_state();
    
    (*focus_)->get_party().game_world().set_lighting(renderer_);

    GLfloat pos[3];
    GLfloat rotate;
    (*focus_)->get_pos(pos, &rotate);
    renderer_.set_pos(pos, (*focus_)->loc());
    
    battle_character::move_map::const_iterator selected_move = moves_.end();
    hex::location selected_hex;
    const_battle_character_ptr selected_character;
    if(highlight_moves_) {
        selected_move = moves_.find(selection_.get_selected_hex());
        if(selected_move != moves_.end()) {
            selected_hex = selected_move->first;
        }
    } else if(highlight_targets_) {
        selected_hex = selection_.get_selected_hex();
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
    

    if(highlight_moves_) {
        typedef battle_character::move_map::const_iterator itor_type;
        for(itor_type itor = moves_.begin(); itor != moves_.end(); ++itor) {
            renderer_.add_highlights(itor->second);
        }
        //draw possible attacks
        if(selected_hex.valid()) {
            foreach(const battle_character_ptr& c, chars_) {
                if(c->loc() == selected_hex) {
                    if((*focus_)->is_enemy(*c)) {
                        const static GLfloat good_color[4] = { 0.0,0.0,1.0,1.0 };
                        const int range = c->get_character().attack_range();
                        GLfloat rotate;
                        
                        GLfloat from[3], to[3];
                        (*focus_)->get_pos(from, &rotate);
                        c->get_pos(to, &rotate);
                        
                        hex::location loc = 
                            map_.tile_in_the_way((*focus_)->loc(), c->loc(), NULL, range);

                        if(!map_.is_loc_on_map(loc)) {
                            renderer_.add_sight_line(from, to, good_color);
                        } else {
                            const static GLfloat bad_color[4] = { 1.0, 1.0, 0.0, 1.0 };
                            const static GLfloat range_color[4] = { 1.0, 0.0, 0.0, 1.0 };
                            GLfloat middle[3];

                            middle[0] = hex::tile::translate_x(loc);
                            middle[1] = hex::tile::translate_y(loc);
                            middle[2] = hex::tile::translate_height(map_.get_tile(loc).height());
                            
                            renderer_.add_sight_line(from, middle, good_color);

                            if(hex::distance_between((*focus_)->loc(), c->loc()) > range) {
                                renderer_.add_sight_line(middle, to, range_color);
                            } else {
                                renderer_.add_sight_line(middle, to, bad_color);
                            }
                        }
                    }
                }
            }
        }
    }

    if(highlight_targets_) {
        std::vector<hex::location> vtargs;
        foreach(hex::location loc, targets_) {
            vtargs.push_back(loc);
        }
        renderer_.add_highlights(vtargs);
    }
    
    if(selected_move != moves_.end()) {
        renderer_.set_path(selected_move->second);
    } else if(focus_ != chars_.end()) {
        renderer_.set_path((*focus_)->movement_plan());
    }
    
    if(map_.is_loc_on_map(selected_hex)) {
        int radius = 0;
        if(current_move_ && current_move_->mod()) {
                radius = current_move_->mod()->radius();
        }
        
        std::vector<hex::location> locs;
        hex::get_locations_in_radius(selected_hex, radius, locs);
        renderer_.add_highlights(locs);
    }
    
    if(map_.is_loc_on_map((*focus_)->loc())) {
        renderer_.add_highlight((*focus_)->loc());
    }
    
    {
        int char_index = 0;
        foreach(const_battle_character_ptr c, chars_) {
            renderer_.add_avatar(c->avatar(), char_index++);
        }
    }
    
    if(missile_) {
        renderer_.add_avatar(missile_->avatar());
    }
    
    return renderer_.draw();
}

void battle::draw_widgets(gui::slider* slider) {
    const SDL_Color white = {0xFF,0xFF,0x0,0};

    hex::location selected_hex = selection_.get_selected_hex();
    const_battle_character_ptr selected_character = selected_char();

    graphics::prepare_raster();
    
    const graphics::texture text = 
        graphics::font::render_text(renderer_.status_text(),20,white); 
    graphics::blit_texture(text,50,50);
   
    if(slider) {
        slider->draw();
    }
    
    
    if(!selected_character) {
        foreach(const battle_character_ptr& c, chars_) {
            if(c->loc() == selected_hex) {
                selected_character = c;
                break;
            }
        }
    }
        
    if(selected_character) {
        if(highlight_targets_ && targets_.count(selected_character->loc())) {
            SDL_Color color = {0xFF,0xFF,0xFF,0xFF};
            std::string desc;
            assert(current_move_);
            get_attack_stats(**focus_, *selected_character, *current_move_, &desc);
            desc = selected_character->status_text() + "\n" + desc;
            graphics::texture text = graphics::font::render_text(desc, 20, color);
            graphics::blit_texture(text,50,100);
        } else {
            SDL_Color color = {0xFF,0xFF,0xFF,0xFF};
            std::vector<graphics::texture> text;
            graphics::font::render_multiline_text(selected_character->status_text(), 20, color, text);
            int x = 50;
            int y = 100;
            foreach(graphics::texture& t, text) {
                graphics::blit_texture(t,x,y);
                y += t.height();
            }
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
}

void battle::begin_animation() {
    stats_dialogs_.clear();
    time_cost_widget_->set_visible(false);
    time_cost_widget_->clear_tracker();
}

void battle::animation_frame(float t, gui::slider* slider) {
    sub_time_ += t;
    
    if(draw()) {
        draw_widgets(slider);
    }
    SDL_GL_SwapBuffers();
    
    camera_controller_.update();
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
    	pump_.process();
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
	stats.damage = std::max<int>(ch.stat_mod_height(DamageStat,height_diff/2) +
	                  attacker.adjust_damage(move.get_stat(DamageStat,attacker)), 1);
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
		const int chance_to_hit = std::min<int>(100, std::max<int>(0, stats.attack - stats.defense));
		*description += formatter() << "Attack: " <<
		                stats.attack << "\nDefense: " <<
						stats.defense <<
						(behind ? " (from behind!)" : "") << "\nChance to hit: " << chance_to_hit <<
						"%\nDamage: " << stats.damage <<
						"\nTime: " << stats.time_taken << "s";
		if(stats.attack > stats.defense &&
		   stats.damage_critical != stats.damage) {
			const int chance_to_critical = std::min<int>(100, std::max<int>(0, stats.attack/2 - stats.defense));
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

	int random = rand()%100;
	const int chance_to_hit = stats.attack - stats.defense;

	const SDL_Rect slider_rect = {100, 650, 800, 100};
	const bool use_slider = preference_sliders() && attacker.is_human();
	bool applied_slider_result = false;
	gui::slider slider(slider_rect, stats.attack, stats.defense, use_slider);

	GLfloat highlight[] = {1.0,0.0,0.0,0.5};
	const GLfloat elapsed_time = stats.time_taken;
	const GLfloat anim_time = use_slider ? slider.duration() : elapsed_time/5.0;
	GLfloat begin_hit = use_slider ? -1.0 : 0.4;
	GLfloat end_hit = use_slider ? -1.0 : 0.8;
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
	for(GLfloat t = 0.0; t < anim_time; t += 0.08) {
		slider.process();
		slider.set_time(t);
		initiative_bar_->focus_character(&attacker, t*(elapsed_time/anim_time));
		if(missile_.get()) {
			missile_->update();
		}
		const GLfloat cur_time = t / anim_time;
		attacker.set_movement_time(cur_time);
		defender.set_movement_time(cur_time);
		attacker.set_attack_time(cur_time);
		if(random > chance_to_hit &&
		   cur_time >= begin_hit && cur_time <= end_hit) {
			defender.set_highlight(highlight);
		}

		if(use_slider && !applied_slider_result && slider.result() != gui::slider::PENDING) {
			if(slider.result() == gui::slider::RED) {
				random /= 2;
			} else if(slider.result() == gui::slider::BLUE) {
				random *= 2;
			}

			begin_hit = t;
			end_hit = t + 0.4;
		}
		
		animation_frame(0.02 * elapsed_time/anim_time, &slider);
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
		graphics::floating_label::add(damage_tex,pos,move,50);
	} else {
		graphics::texture damage_tex = graphics::font::render_text("miss!", 20, blue);
		graphics::floating_label::add(damage_tex,pos,move,50);
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
#if 0
    // FIXME: move particle emitter to renderer usw

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
#endif

    assert(move.mod());
    const battle_modification& mod = *move.mod();
    caster.set_time_until_next_move(time_to_perform);
    caster.use_energy(move.energy_required());
    const battle_modification::TARGET_TYPE type = mod.target();
    const int radius = mod.radius();
    std::vector<hex::location> locs;
    std::vector<battle_character_ptr> affected_chars;
    get_locations_in_radius(target, radius, locs);
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
	hex::get_locations_in_radius((*focus_)->loc(), mod->range(), locs);
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

bool battle::listener::process_event(const SDL_Event& event, bool claimed) {
    if(claimed) {
        claimed |= handle_stats_dialogs(event, claimed);
        return claimed;
    }

    switch(event.type) {
    case SDL_KEYDOWN:
        if(battle_->highlight_targets_ &&
           (event.key.keysym.sym == SDLK_RETURN ||
            event.key.keysym.sym == SDLK_SPACE)) {
            assert(battle_->current_move_);
            if(battle_->current_move_->can_attack()) {
                battle_->turn_done_ = true;
                battle_character_ptr target_char = battle_->selected_char();
                battle_->attack_character(**(battle_->focus_), *target_char, 
                                          *(battle_->current_move_));
            } else {
                hex::location loc = battle_->selection_.get_selected_hex();
                if(battle_->map_.is_loc_on_map(loc)) {
                    battle_->turn_done_ = true;
                    assert(battle_->current_move_->mod());
                    battle_->target_mod(**(battle_->focus_), loc, *(battle_->current_move_));
                }
            }
            claimed = true;
        } else if(battle_->current_move_ &&
                  (event.key.keysym.sym == SDLK_ESCAPE ||
                   event.key.keysym.sym == SDLK_RETURN ||
                   event.key.keysym.sym == SDLK_SPACE)) {
            if(battle_->move_done_) {
                battle_->turn_done_ = true;
            } else {
                battle_->current_move_.reset();
                battle_->menu_.reset(new gui::battle_menu(*battle_, *c_));
                battle_->add_widget(battle_->menu_);
                battle_->highlight_targets_ = false;
                battle_->highlight_moves_ = false;
            }
            claimed = true;
        } else if(battle_->highlight_targets_) {
            switch(event.key.keysym.sym) {
            case SDLK_RIGHT:
                ++(battle_->keyed_selection_);
                claimed = true;
                break;
            case SDLK_LEFT:
                --(battle_->keyed_selection_);
                claimed = true;
                break;
            }
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        claimed = handle_mouse_button_down(event.button);
        break;
    case SDL_MOUSEMOTION:
        handle_time_cost_popup();
        break;
    }

    claimed |= handle_stats_dialogs(event, claimed);

    return claimed;
}

void battle::listener::handle_time_cost_popup()
{
    battle_character_ptr attacker = *(battle_->focus_);
    assert(attacker);
    assert(battle_->initiative_bar_);
#if 0
    assert(battle_->time_cost_widget_);
#endif
    
    if(battle_->highlight_moves_ ) {
        const battle_character::move_map::const_iterator move = 
            battle_->moves_.find(battle_->selection_.get_selected_hex());
        if(move != battle_->moves_.end()) {
            int cost = attacker->route_cost(move->second);
            battle_->initiative_bar_->focus_character(attacker.get(), cost);
#if 0
            battle_->time_cost_widget_->set_tracker(&(battle_->hex_tracker_));
            battle_->time_cost_widget_->set_time_cost(cost);
            battle_->time_cost_widget_->set_visible(true);
#endif
            return;
        } else {
            battle_->initiative_bar_->focus_character(attacker.get(), 0);
        }
    } else if(battle_->highlight_targets_) {
        battle_character_ptr defender = battle_->selected_char();
        assert(battle_->current_move_);
        if(battle_->current_move_->can_attack() && defender) {
            /* attack cost */
            attack_stats stats = 
                battle_->get_attack_stats(*attacker, *defender, *(battle_->current_move_));
#if 0
            battle_->time_cost_widget_->set_tracker(&(defender->loc_tracker()));
            battle_->time_cost_widget_->set_time_cost(stats.time_taken);
            battle_->time_cost_widget_->set_visible(true);
#endif
            battle_->initiative_bar_->focus_character(attacker.get(), stats.time_taken);
            return;
        } else {
            /* SPELL cost */
            hex::location loc = battle_->selection_.get_selected_hex();
            if(battle_->map_.is_loc_on_map(loc)) {
                /* move cost */
                battle_->time_cost_widget_->set_tracker(&(battle_->hex_tracker_));
                const int cost = battle_->current_move_->get_stat("initiative",*attacker);
                battle_->initiative_bar_->focus_character(attacker.get(), cost);
#if 0
                battle_->time_cost_widget_->set_time_cost(cost);
                battle_->time_cost_widget_->set_visible(true);
#endif
                return;
            }
        }
    }

#if 0    
    battle_->time_cost_widget_->clear_tracker();
    battle_->time_cost_widget_->set_visible(false);
#endif
}

bool battle::listener::handle_stats_dialogs(const SDL_Event& e, bool claimed) {
    const bool has_dialogs = !battle_->stats_dialogs_.empty();
    
    if(claimed) {
        if(has_dialogs) {
            battle_->stats_dialogs_.clear();
        }
        return claimed;
    }
    
    bool clear;
    switch(e.type) {
    case SDL_KEYDOWN:
        if(has_dialogs) {
            clear = true;
            claimed = true;
        } else {
            clear = false;
            claimed = false;
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if(e.button.button != SDL_BUTTON_RIGHT) {
            if(has_dialogs) {
                clear = true;
                claimed = true;
            } else {
                clear = false;
                claimed = false;
            }
            break;
        }
        {
            battle_character_ptr target_char = battle_->mouse_selected_char();
            if(target_char) {
                claimed = true;
                clear = false;
                
                game_dialogs::mini_stats_dialog_ptr ptr;
                
                std::map<battle_character_ptr,game_dialogs::mini_stats_dialog_ptr>::iterator i =
                    battle_->stats_dialogs_.find(target_char);
                if(i == battle_->stats_dialogs_.end()) {
                    ptr.reset(new game_dialogs::mini_stats_dialog(target_char, 200, 100));
                    battle_->stats_dialogs_[target_char] = ptr;
                    ptr->show();
                    ptr->set_frame(gui::frame_manager::make_frame(ptr, "mini-char-battle-stats"));
                } else {
                    battle_->stats_dialogs_.erase(i);
                }
            } else {
                if(has_dialogs) {
                    claimed = true;
                    clear = true;
                } else {
                    claimed = false;
                    clear = false;
                }
            }
        }
        break;
    default:
        clear = false;
        claimed = false;
        break;
    }
    
    if(clear) {
        battle_->stats_dialogs_.clear();
    }
    
    return claimed;
}

bool battle::listener::handle_mouse_button_down(const SDL_MouseButtonEvent& e)
{
    bool claimed = false;

	if(e.button == SDL_BUTTON_LEFT) {
		if(battle_->highlight_moves_) {
			const battle_character::move_map::const_iterator move = 
                battle_->moves_.find(battle_->selection_.get_selected_hex());
			if(move != battle_->moves_.end()) {
				battle_->move_character(**(battle_->focus_), move->second);
                battle_->turn_done_ = !battle_->enter_attack_mode();
				battle_->move_done_ = true;
                claimed = true;
			}
		} else if(battle_->highlight_targets_ && battle_->current_move_->can_attack()) {
			battle_character_ptr target_char = battle_->selected_char();

			if(target_char && battle_->targets_.count(target_char->loc())) {
				battle_->turn_done_ = true;
				battle_->attack_character(**(battle_->focus_), *target_char, *(battle_->current_move_));
				if(battle_->move_done_) {
					battle_->turn_done_ = true;
				}

				battle_->highlight_targets_ = false;
                battle_->highlight_moves_ = false;
				battle_->targets_.clear();
                claimed =  true;
			}
		} else if(battle_->highlight_targets_) {
			hex::location loc = battle_->selection_.get_selected_hex();
			if(battle_->map_.is_loc_on_map(loc) && battle_->targets_.count(loc)) {
				battle_->turn_done_ = true;
				battle_->target_mod(**(battle_->focus_), loc, *(battle_->current_move_));
                claimed = true;
			}
		}
	}
    return claimed;
}

}
