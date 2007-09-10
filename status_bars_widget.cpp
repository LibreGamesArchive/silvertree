#include <math.h>

#include "character.hpp"
#include "status_bars_widget.hpp"

namespace game_dialogs {

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
	if(rch.dead()) {
		return;
	}

	GLfloat pos[4];
	ch_->loc_tracker().get_bbox(pos);
	
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

	GLfloat r_hitpoints = static_cast<GLfloat>(rch.hitpoints());

	if(hitpoints_ >= 0) {
		smooth_transition(hitpoints_, old_hitpoints_, r_hitpoints);
	} else {
		hitpoints_ = r_hitpoints;
	}

	GLfloat health_angle = health_max_angle*hitpoints_ / max_hitpoints_;
	if(health_angle < 20) {
		health_angle = 20;
	} else if(health_angle > 100) {
		health_angle = 100;
	}

	GLfloat fastest = HUGE_VALF;
	GLfloat second_fastest = HUGE_VALF;
	for(std::vector<game_logic::battle_character_ptr>::const_iterator i = b_.participants().begin();
	    i != b_.participants().end(); ++i) {
		GLfloat ready = (*i)->ready_to_move_at();
		GLfloat move_time = (*i)->get_movement_time();
		if(move_time > 0.0) ready += move_time;
		if(ready < fastest) {
			second_fastest = fastest;
			fastest = ready;
		}  else if(ready < second_fastest) {
			second_fastest = ready;
		}
	}

	GLfloat time_to_move = ch_->ready_to_move_at();
	GLfloat move_time = ch_->get_movement_time();
	if(move_time > 0) {
		time_to_move +=  move_time;
	}
	if(time_to_move == fastest) {
		if(second_fastest == HUGE_VALF) {
			time_to_move = 0;
		}
		time_to_move -= second_fastest;
	} else {
		time_to_move -= fastest;
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


	GLfloat center_x = (pos[0] + pos[2])/2;
	GLfloat center_y = (pos[1] + pos[3])/2;

	GLfloat radius = 0;

	for(int i=0;i<2;++i) {
		GLfloat dist_x = (pos[i*2] - center_x);
		GLfloat dist_y = (pos[i*2+1] - center_y);
		GLfloat dist = dist_x*dist_x + dist_y*dist_y;
		if(dist > radius) {
			radius = dist;
		}
	}
	radius = sqrt(radius);

	GLUquadric* quad =  gluNewQuadric();

	try {
		glDisable(GL_TEXTURE_2D);
		glPushMatrix();
		glTranslatef(center_x, center_y, 0);
		
		if(health_angle > 0) {
			glColor4ub(damage_color.r, damage_color.g, damage_color.b, 180);
			gluPartialDisk(quad, radius*9/10, radius*11/10, 16, 1, 
				       -90-health_max_angle/2 + health_angle, +(health_max_angle-health_angle));
			glColor4ub(bar_color.r, bar_color.g, bar_color.b, 180);
			gluPartialDisk(quad, radius*9/10, radius*11/10, 16, 1, 
				       -90-health_max_angle/2, +health_angle);
		}
		if(time_to_move > 0) {
			GLfloat big_units = time_to_move/10;
			int big_segments = static_cast<int>(big_units);
			GLfloat small_units = time_to_move - big_segments*10;
			int small_segments = static_cast<int>(small_units);
			glColor4ub(time_color.r, time_color.g, time_color.b, 180);
			
			GLfloat small_units_angle = small_units * 14 + (small_units - small_segments);

			for(int i =0;i<small_segments;++i) {
				gluPartialDisk(quad, radius*12/10, radius*14/10, 16, 1, 
					       -90 + (small_units_angle/2) - i*14, -10); 
			}
			gluPartialDisk(quad, radius*12/10, radius*14/10, 16, 1, 
				       -90 + (small_units_angle/2) - small_segments*14, 
				       -10*(small_units - small_segments));

			if(big_segments > 0) {
				if(big_units  > 10) big_units = 10;
				if(big_segments > 10) big_segments = 10;
				for(int i=0;i<big_segments;++i) {
					gluPartialDisk(quad, radius*15/10, radius*16/10, 16, 1, 
						       -40.0 - (10.0/big_segments) - i*100.0/big_segments, 
						       -90.0/(big_segments)); 
				}
			}
		}

		glPopMatrix();
		glEnable(GL_TEXTURE_2D);

	} catch(...) {
		gluDeleteQuadric(quad);
		throw;
	}
	gluDeleteQuadric(quad);
}

}
