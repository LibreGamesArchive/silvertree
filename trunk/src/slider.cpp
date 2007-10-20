
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

slider::slider(const SDL_Rect& rect, int attack, int defense,
               bool use_slider)
 : rect_(rect), attack_(attack), defense_(defense),
   time_(0.0), stopped_(-1.0),
   use_slider_(use_slider), result_(PENDING)
{}

GLfloat slider::duration() const
{
	return (use_slider_ ? lead_time : 0.0) + 20.0/defense_;
}

GLfloat slider::critical_section() const
{
	return GLfloat(attack_)*0.001;
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
	const GLfloat pos = time_ < lead_time ? 0.0 : (time_ - lead_time)/(duration() - lead_time);
	SDL_Event event;
	while(SDL_PollEvent(&event)) {
		switch(event.type) {
		case SDL_KEYDOWN:
			if(stopped_ < 0.0 && pos > 0.0) {
				stopped_ = pos;
			}
			break;
		}
	}
}

slider::RESULT slider::result() const
{
	if(result_ != PENDING) {
		return result_;
	}

	int random = rand()%(attack_+defense_);
	if(!use_slider_) {
		if(random < defense_) {
			return result_ = MISS;
		} else {
			random -= defense_;
			if(random >= attack_ - attack_/5) {
				return result_ = CRITICAL;
			} else {
				return result_ = HIT;
			}
		}
	}

	if(stopped_ < 0.0) {
		if(time_ >= duration()) {
			return result_ = FUMBLE;
		} else {
			return PENDING;
		}
	} else if (stopped_ < hit_section) {
		return result_ = random < defense_ ? MISS : HIT;
	} else if (stopped_ < hit_section + critical_section()) {
		return result_ = CRITICAL;
	} else {
		random /= 2;
		return result_ = random < defense_ ? MISS : FUMBLE;
	}
}

}
