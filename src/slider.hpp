
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SLIDER_HPP_INCLUDED
#define SLIDER_HPP_INCLUDED

#include <GL/glew.h>

#include "SDL.h"

#include "input.hpp"

namespace gui {

class slider : public input::listener
{
public:
	slider(input::pump& pump, const SDL_Rect& rect,
	       int attack, int defense, bool use_slider);
	~slider();

	GLfloat duration() const;
	GLfloat critical_section() const;
	void set_time(GLfloat time) { time_ = time; }
	void draw();
	void process();
	virtual bool process_event(const SDL_Event& e, bool is_claimed);

	enum RESULT { PENDING, BLUE, YELLOW, RED };
	RESULT result() const;

private:
	input::pump& pump_;
	SDL_Rect rect_;
	int attack_;
	int defense_;
	GLfloat time_;
	GLfloat stopped_;
	bool use_slider_;
	mutable RESULT result_;
};

}

#endif
