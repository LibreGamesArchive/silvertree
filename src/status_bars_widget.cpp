#include <math.h>

#include "character.hpp"
#include "status_bars_widget.hpp"

namespace game_dialogs {

namespace {

void draw_anchored_expanding_time_bar(GLfloat time, GLfloat accounted_time, int units, GLfloat range_start_angle,
					     GLfloat range_length, GLfloat proportion_ticks, GLfloat inner_radius,
					     GLfloat thickness, const SDL_Color& main_color, const SDL_Color& accounted_color,
					     GLUquadric *quad)
{
	if(time <= 0) {
		return;
	}
	if(time > 10.0) {
		time = 10.0;
	}

	const int segments = static_cast<int>(time);

	const GLfloat angle_per_tick = range_length / units;
	const GLfloat drawn_tick_angle = angle_per_tick * proportion_ticks;
	const GLfloat skip_angle = (angle_per_tick - drawn_tick_angle)/2;
	const GLfloat outer_radius = inner_radius + thickness;
	//const GLfloat total_angle = segments * angle_per_tick + (time - segments)*angle_per_tick;

	glColor4ub(main_color.r, main_color.g, main_color.b, 255);

	GLfloat px_radial = 180.0/(M_PI * (inner_radius + thickness/2));
	GLfloat px_axial = 1.5;

	if(drawn_tick_angle < 0) {
		px_radial = -px_radial;
	}
	if(segments > 0) {
		for(int i =0;i<segments;++i) {
			glColor4ub(0, 0, 0, 255);
			gluPartialDisk(quad, inner_radius-px_axial, outer_radius+px_axial, 16, 2,
				       range_start_angle + i*angle_per_tick + skip_angle - px_radial,
				       drawn_tick_angle + 2*px_radial);


			GLfloat acc = 0.0;
			glColor4ub(main_color.r, main_color.g, main_color.b, 255);
			if(accounted_time > i) {
				/* the accounted time reached this far */
				glColor4ub(accounted_color.r, accounted_color.g, accounted_color.b, 255);
				if(accounted_time < i+1) {
					/* it doesn't cover the whole segment though */
					acc = accounted_time - i;
					gluPartialDisk(quad, inner_radius, outer_radius, 16, 1,
						       range_start_angle + i*angle_per_tick + skip_angle,
						       drawn_tick_angle * acc);
					glColor4ub(main_color.r, main_color.g, main_color.b, 255);
				}
			}
			gluPartialDisk(quad, inner_radius, outer_radius, 16, 1,
				       range_start_angle + i*angle_per_tick + drawn_tick_angle*acc + skip_angle,
				       drawn_tick_angle * (1-acc));
		}
	}

	glColor4ub(255, 255, 255, 255);
}

void draw_time_bars(GLfloat time, GLfloat accounted_time, GLfloat start, GLfloat thickness,
			   GLfloat start_ang, GLfloat range, int units,
			   const SDL_Color& main_color, const SDL_Color& accounted_color,
			   GLUquadric *quad)
{
	//for the moment, the character time bars are removed in favor of the single initiative bar.
	return;
	if(time == 0.0) {
		return;
	}

	const int max_bar = static_cast<int>(time / units);
	GLfloat cur_time = time;
	GLfloat cur_accounted = accounted_time;

	for(int cur_bar = 0; cur_bar < max_bar; ++cur_bar) {
		GLfloat bar_accounted;
		if(cur_accounted <= units) {
			bar_accounted = cur_accounted;
		} else {
			bar_accounted = units;
		}
		const GLfloat bar_inner = start + (3/2)*cur_bar*thickness;
		draw_anchored_expanding_time_bar(units, bar_accounted, units, start_ang, range, 10.0/14.0, bar_inner,
						 thickness, main_color, accounted_color, quad);
		cur_time -= units;
		cur_accounted -= bar_accounted;
		if(cur_accounted < 0) cur_accounted = 0;
	}
	draw_anchored_expanding_time_bar(cur_time, cur_accounted, units, start_ang, range, 10.0/14.0,
					 start + (3/2)*max_bar*thickness, thickness, main_color, accounted_color, quad);

}

inline void smooth_transition(GLfloat& x, GLfloat& prev, GLfloat cur) {
	if(cur != x) {
		GLfloat step;
		if(prev >= 0) {
			step = (cur - prev)/50;
			if ((cur - x - step) * (cur - prev) < 0) {
				step = (cur - x);
			}
			/* force a minimum transition rate */
			if(step < 0 && step > -0.1) {
				step = -0.1;
			} else if(step > 0 && step < 0.1) {
				step = 0.1;
			}
		} else {
			step = (cur - x);
			if(step < -0.1) step = -0.1;
			if(step >  0.1) step =  0.1;
		}

		x += step;
	} else {
		prev = cur;
	}
}

inline GLfloat calc_time_to_move(const game_logic::battle_character_ptr ch, const game_logic::battle& b) {
	GLfloat fastest = HUGE_VALF;
	for(std::vector<game_logic::battle_character_ptr>::const_iterator i = b.participants().begin();
	    i != b.participants().end(); ++i) {
		if((*i) == ch) continue;
		GLfloat ready = (*i)->ready_to_move_at();
		if(ready < fastest) {
			fastest = ready;
		}
	}
	GLfloat ret = ch->ready_to_move_at();

	if(ch == b.active_character()) {
		/* i am the char moving */
		if(fastest == HUGE_VALF) {
			ret = 0;
		} else {
			ret = b.current_time() + b.animation_time() - fastest ;
		}
	} else {
		ret -= b.current_time() + b.animation_time();
	}
	return ret;
}

struct release_quadric { void operator()(GLUquadric* quad) const { gluDeleteQuadric(quad); } };
typedef util::scoped_resource<GLUquadric*, release_quadric> scoped_quadric;

}


void status_bars_widget::handle_draw() const {
	const game_logic::character& rch = ch_->get_character();
	if(rch.dead() && hitpoints_ == 0) {
		return;
	}

	GLfloat r_max_hitpoints = static_cast<GLfloat>(rch.max_hitpoints());

	if(max_hitpoints_ >= 0) {
		smooth_transition(max_hitpoints_, old_max_hitpoints_, r_max_hitpoints);
	} else {
		max_hitpoints_ = r_max_hitpoints;
	}

	GLfloat health_max_angle = static_cast<int>(100*(1 - exp(-max_hitpoints_/20.0)));
	if(health_max_angle < 20) {
		health_max_angle = 20;
	}

	GLfloat r_hitpoints;
	if(!rch.dead()) {
		r_hitpoints = static_cast<GLfloat>(rch.hitpoints());
	} else {
		r_hitpoints = 0;
	}

	if(hitpoints_ >= 0) {
		smooth_transition(hitpoints_, old_hitpoints_, r_hitpoints);
	} else {
		hitpoints_ = r_hitpoints;
	}

	GLfloat health_angle = health_max_angle*hitpoints_ / max_hitpoints_;
	if(health_angle > 100) {
		health_angle = 100;
	}

	int ready = ch_->ready_to_move_at();
	GLfloat time_remaining = calc_time_to_move(ch_, b_);
	GLfloat time;
	if(ch_ == b_.active_character()) {
		if(b_.animation_time() == 0) {
			/* we're at the start of our move */
			if(cur_move_time_ != ready) {
				/* update move records */
				prev_move_time_ = cur_move_time_;
				cur_move_time_ = ready;
			}
			/* find a reasonable amount of time to show */
			if(prev_move_time_ == -HUGE_VALF) {
				time = 0;
			} else {
				time = cur_move_time_ - prev_move_time_;
			}
			/* bars are solid cyan - we're about to move */
			time_remaining = 0;
		} else {
			/* we're in the middle of our move */
			time = b_.animation_time();
			//time_remaining = time - time_remaining;
			if(time_remaining < 0) {
				time_remaining = 0;
			}
		}
	} else {
		/* we are not the active character, so not moving */
		if(cur_move_time_ == -HUGE_VALF) {
			/* have not moved yet, only happens once */
			time = time_remaining;
			cur_move_time_ = b_.current_time() - time_remaining;
		} else {
			time  = ready - cur_move_time_;
		}
		if(time_remaining < 0) {
			time_remaining = 0;
		}
	}

	SDL_Color energy_color = {255,0,255};
	SDL_Color time_waited_color = {1,195,255};
	SDL_Color time_to_go_color = {13,20,108};
	SDL_Color healthy_color = { 15, 87, 5 };
	SDL_Color damaged_color = { 255, 0, 0 };

	GLfloat bcircle[3];
	ch_->loc_tracker().get_bcircle(bcircle);

	scoped_quadric quad(gluNewQuadric());

	glDisable(GL_TEXTURE_2D);
	glPushMatrix();
	glTranslatef(bcircle[0], bcircle[1], 0);

	GLfloat px_radius = bcircle[2] * 0.5 * M_PI/180.0;

	// draw the character's magic energy
	if(ch_->energy() > 0) {
		glColor4ub(energy_color.r, energy_color.g, energy_color.b, 255);
		gluPartialDisk(quad.get(), bcircle[2]*6/10-1, bcircle[2]*8/10+1, 16, 1,
		               -20, -20 - std::min<int>(ch_->energy(),25)*5);
	}

	glColor4ub(0,0,0,255);
	gluPartialDisk(quad.get(), bcircle[2]*9/10-1, bcircle[2]*11/10+1, 16, 1,
		       -20 + px_radius, -(health_max_angle + 2*px_radius));

	if(health_max_angle - health_angle > 0) {
		glColor4ub(damaged_color.r, damaged_color.g, damaged_color.b, 255);
		gluPartialDisk(quad.get(), bcircle[2]*9/10, bcircle[2]*11/10, 16, 1,
			       -20-health_angle, -(health_max_angle - health_angle));
	}
	if(health_angle > 0) {
		glColor4ub(healthy_color.r, healthy_color.g, healthy_color.b, 255);
		gluPartialDisk(quad.get(), bcircle[2]*9/10, bcircle[2]*11/10, 16, 1,
			       -20, -health_angle);
	}
	draw_time_bars(time, time-time_remaining, bcircle[2]*12/10, bcircle[2]*2/10, -20, -140, 10,
		       time_to_go_color, time_waited_color, quad.get());

	glPopMatrix();
	glEnable(GL_TEXTURE_2D);

	glColor4ub(255, 255, 255, 255);
}

void time_cost_widget::handle_draw() const {
	if(!tracker_) {
		return;
	}

	GLfloat bcircle[3];
	tracker_->get_bcircle(bcircle);

	const SDL_Color cost_color = { 255, 150, 255 };
	const SDL_Color free_color = { 255, 255, 255 };

	GLfloat free_time = calc_time_to_move(b_.active_character(), b_);
	if(free_time < 0) {
		free_time = -free_time;
	} else {
		free_time = 0;
	}

	scoped_quadric quad(gluNewQuadric());
	glDisable(GL_TEXTURE_2D);
	glPushMatrix();
	glTranslatef(bcircle[0], bcircle[1], 0);

	draw_time_bars(time_cost_, free_time, bcircle[2]/3, bcircle[2]/10, 180, 360, 10,
		       cost_color, free_color, quad.get());

	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
}

}
