#include <math.h>

#include "character.hpp"
#include "status_bars_widget.hpp"

namespace game_dialogs {

namespace {

inline void draw_centered_expanding_time_bar(GLfloat time, int units, GLfloat range_center_angle, GLfloat range_length, 
					    GLfloat proportion_ticks, GLfloat inner_radius, GLfloat thickness, 
					    const SDL_Color& color, Uint8 alpha, GLUquadric *quad) 
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
	const GLfloat total_angle = segments * angle_per_tick + (time - segments)*angle_per_tick;

	glColor4ub(color.r, color.g, color.b, alpha);
			
	
	if(segments > 0) {
		for(int i =0;i<segments;++i) {
			gluPartialDisk(quad, inner_radius, outer_radius, 16, 1, 
				       range_center_angle - (total_angle/2) + i*angle_per_tick + skip_angle, 
				       drawn_tick_angle); 
		}
	}
	if(time != static_cast<GLfloat>(segments)) {
		gluPartialDisk(quad, inner_radius, outer_radius, 16, 1, 
			       range_center_angle - (total_angle/2) + segments*angle_per_tick + skip_angle, 
			       drawn_tick_angle*(time - segments));
	}		
}

inline void draw_centered_scaling_time_bar(GLfloat time, int units, GLfloat range_center_angle, GLfloat range_length, 
					   GLfloat proportion_ticks, GLfloat inner_radius, GLfloat thickness, 
					   const SDL_Color& color, Uint8 alpha, GLUquadric *quad) 
{
	int segments = static_cast<int>(time);
	if(segments < 1) {
		return;
	}
	if(segments > 10) {
		segments = 10;
	}
	
	const GLfloat angle_per_tick = range_length / segments;
	const GLfloat drawn_tick_angle = angle_per_tick * proportion_ticks;
	const GLfloat skip_angle = (angle_per_tick - drawn_tick_angle)/2;
	const GLfloat outer_radius = inner_radius + thickness;

	glColor4ub(color.r, color.g, color.b, alpha);
			
	if(segments > 0) {
		for(int i=0;i<segments;++i) {
			gluPartialDisk(quad, inner_radius, outer_radius, 16, 1, 
				       range_center_angle - (range_length/2) + i*angle_per_tick + skip_angle, 
				       drawn_tick_angle); 
		}
	}
}

inline void draw_time_bars(GLfloat time, GLfloat start, GLfloat thickness, GLfloat start_ang, GLfloat range,
			   int units, const SDL_Color& color, Uint8 alpha, GLUquadric *quad) 
{
	int cur_bar = 1;
	GLfloat divisor = 1.0;
	while(time / divisor > units) {
		divisor *= units;
		++cur_bar;
	}
	GLfloat cur_time = time;
	while(divisor > 1.0) {
		GLfloat bar_time = cur_time/divisor;
		const GLfloat bar_inner = start + (1 + 2*(cur_bar-1))*thickness;
		const GLfloat bar_thickness = thickness/2;
		draw_centered_scaling_time_bar(bar_time, units, start_ang, range, 10.0/14.0, bar_inner,
					       bar_thickness, color, alpha, quad);
		--cur_bar;
		cur_time -= static_cast<int>(bar_time)*divisor;
		divisor /= units;
	}

	draw_centered_expanding_time_bar(cur_time, units, start_ang, range, 10.0/14.0, start, 
					 thickness, color, alpha, quad);
}

inline void get_bcircle(const GLfloat* bbox, GLfloat* bcircle) {
	bcircle[0] = (bbox[0] + bbox[2])/2;
	bcircle[1] = (bbox[1] + bbox[3])/2;
	bcircle[2] = 0;

	for(int i=0;i<2;++i) {
		GLfloat dist_x = (bbox[i*2] - bcircle[0]);
		GLfloat dist_y = (bbox[i*2+1] - bcircle[1]);
		GLfloat dist = dist_x*dist_x + dist_y*dist_y;
		if(dist > bcircle[2]) {
			bcircle[2] = dist;
		}
	}
	bcircle[2] = sqrt(bcircle[2]);
}
}


void status_bars_widget::smooth_transition(GLfloat& x, GLfloat& prev, GLfloat cur) const {
	if(cur != x) {
		GLfloat step;
		if(prev >= 0) {
			step = (cur - prev)/50;
			if ((cur - x - step) * (cur - prev) < 0) {
				step = (cur - x);
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

void status_bars_widget::handle_draw() const {
	const game_logic::character& rch = ch_->get_character();
	if(rch.dead() && hitpoints_ == 0) {
		return;
	}

	GLfloat bbox[4];
	ch_->loc_tracker().get_bbox(bbox);
	
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

	GLfloat fastest = HUGE_VALF;
	GLfloat second_fastest = HUGE_VALF;
	for(std::vector<game_logic::battle_character_ptr>::const_iterator i = b_.participants().begin();
	    i != b_.participants().end(); ++i) {
		GLfloat ready = (*i)->ready_to_move_at();
		if(ready < fastest) {
			second_fastest = fastest;
			fastest = ready;
		}  else if(ready < second_fastest) {
			second_fastest = ready;
		}
	}

 	GLfloat time_to_move = ch_->ready_to_move_at();

	if(time_to_move == fastest) {
		if(second_fastest == HUGE_VALF) {
			time_to_move = 0;
		} else {
			time_to_move = b_.current_time() + b_.animation_time() - second_fastest ;
		} 
	} else {
		time_to_move -= b_.current_time() + b_.animation_time();
	}

	SDL_Color time_color;
	bool ahead = false;
	if(time_to_move < 0) {
		time_to_move = -time_to_move;
		ahead = true;
	}
	if(ahead) {
		time_color.r = 100;
		time_color.g = 255;
		time_color.b = 100;
	} else {
		time_color.r = 100;
		time_color.g = 100;
		time_color.b = 255;
	}

	SDL_Color bar_color;
	if(old_hitpoints_ >= 0 && old_hitpoints_ != r_hitpoints) {
		bar_color.r = 255;
		bar_color.g = 255;
		bar_color.b = 255;
	} else {
		bar_color.r = 10;
		bar_color.g = 150;
		bar_color.b = 10;
	} 

	SDL_Color damage_color = { 255, 10, 10 };

	GLfloat bcircle[3];
	get_bcircle(bbox, bcircle);

	GLUquadric* quad =  gluNewQuadric();

	try {
		glDisable(GL_TEXTURE_2D);
		glPushMatrix();
		glTranslatef(bcircle[0], bcircle[1], 0);
		
		if(health_angle > 0) {
			glColor4ub(damage_color.r, damage_color.g, damage_color.b, 180);
			gluPartialDisk(quad, bcircle[2]*9/10, bcircle[2]*11/10, 16, 1, 
				       -90-health_max_angle/2 + health_angle, +(health_max_angle-health_angle));
			glColor4ub(bar_color.r, bar_color.g, bar_color.b, 180);
			gluPartialDisk(quad, bcircle[2]*9/10, bcircle[2]*11/10, 16, 1, 
				       -90-health_max_angle/2, +health_angle);
		}
		draw_time_bars(time_to_move, bcircle[2]*12/10, bcircle[2]*2/10, -90, 140, 10, time_color, 180, quad);

		glPopMatrix();
		glEnable(GL_TEXTURE_2D);

	} catch(...) {
		gluDeleteQuadric(quad);
		throw;
	}
	gluDeleteQuadric(quad);
}

void time_cost_widget::handle_draw() const {
	if(!tracker_) {
		return;
	}

	GLfloat bbox[4];
	tracker_->get_bbox(bbox);
	GLfloat bcircle[3];
	get_bcircle(bbox, bcircle);
	
	const SDL_Color cost_color = { 255, 200, 255 };

	GLUquadric *quad = gluNewQuadric();
	try {
		glDisable(GL_TEXTURE_2D);
		glPushMatrix();
		glTranslatef(bcircle[0], bcircle[1], 0);
		
		draw_time_bars(time_cost_, bcircle[2]/3, bcircle[2]/10, 180, 360, 10, cost_color, 180, quad);

		glPopMatrix();
		glEnable(GL_TEXTURE_2D);

		
	} catch(...) {
		gluDeleteQuadric(quad);
	}
	gluDeleteQuadric(quad);
}

}
