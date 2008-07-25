
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "slider.hpp"

#include <iostream>

namespace gui {

namespace {
	const GLfloat lead_time = 2.0;
	const GLfloat hit_section = 0.8;
}

slider::slider(input::pump& pump, const SDL_Rect& rect,
               int attack, int defense, bool use_slider)
 : pump_(pump),
   rect_(rect), attack_(attack), defense_(std::max<int>(4,defense)),
   time_(0.0), stopped_(-1.0),
   use_slider_(use_slider), result_(PENDING)
{
	std::cerr << "register listener\n";
	pump_.register_listener(this);
}

slider::~slider()
{
	pump_.deregister_listener(this);
}

GLfloat slider::duration() const
{
	return (use_slider_ ? lead_time : 0.0) + 50.0/defense_;
}

GLfloat slider::critical_section() const
{
	return GLfloat(attack_)*0.01;
}

void slider::draw()
{
	if(!use_slider_) {
		return;
	}

	glPushMatrix();
	glLoadIdentity();
	glDisable(GL_TEXTURE_2D);
	glColor3ub(0xFF,0xFF,0x00);
	glRectf(rect_.x, rect_.y,
	        rect_.x+rect_.w*hit_section, rect_.y+rect_.h);
	glColor3ub(0xFF,0x00,0x00);
	glRectf(rect_.x+rect_.w*hit_section, rect_.y,
	        rect_.x+rect_.w*(hit_section+critical_section()),
			rect_.y+rect_.h);
	glColor3ub(0x00,0xFF,0x00);
	glRectf(rect_.x+rect_.w*(hit_section+critical_section()), rect_.y,
	        rect_.x+rect_.w, rect_.y+rect_.h);

	glColor3ub(0xFF,0xFF,0xFF);
	GLfloat pos = time_ < lead_time ? 0.0 : (time_ - lead_time)/(duration() - lead_time);
	if(stopped_ >= 0.0) {
		pos = stopped_;
	}
	glRectf(rect_.x + rect_.w*pos - 2.0, rect_.y,
	        rect_.x + rect_.w*pos + 2.0, rect_.y+rect_.h);
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();
}

void slider::process()
{
}

bool slider::process_event(const SDL_Event& e, bool is_claimed)
{
	std::cerr << "slider intercept\n";
	if(!use_slider_) {
		return false;
	}

	if(e.type == SDL_KEYDOWN) {
		const GLfloat pos = time_ < lead_time ? 0.0 : (time_ - lead_time)/(duration() - lead_time);
		if(stopped_ < 0.0 && pos > 0.0) {
			stopped_ = pos;
		}
	}

	std::cerr << "slider return\n";
	return true;
}

slider::RESULT slider::result() const
{
	if(result_ != PENDING) {
		return result_;
	}

	if(!use_slider_) {
		return YELLOW;
	}

	if(stopped_ < 0.0) {
		if(time_ >= duration()) {
			return result_ = BLUE;
		} else {
			return PENDING;
		}
	} else if (stopped_ < hit_section) {
		return result_ = YELLOW;
	} else if (stopped_ < hit_section + critical_section()) {
		return result_ = RED;
	} else {
		return result_ = BLUE;
	}
}

}
