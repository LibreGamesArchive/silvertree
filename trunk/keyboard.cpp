
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "keyboard.hpp"
#include "SDL.h"

namespace keyboard
{

hex::DIRECTION dir(hex::DIRECTION orientation)
{
	const Uint8* keys = SDL_GetKeyState(NULL);

	if(keys[SDLK_LCTRL] || keys[SDLK_RCTRL]) {
		return hex::NULL_DIRECTION;
	}

	int dir = -1;
	if(keys[SDLK_KP7] || keys[SDLK_LEFT] && keys[SDLK_UP]) {
		dir = 1;
	} else if(keys[SDLK_KP9] || keys[SDLK_RIGHT] && keys[SDLK_UP]) {
		dir = 5;
	} else if(keys[SDLK_KP1] || keys[SDLK_LEFT] && keys[SDLK_DOWN]) {
		dir = 2;
	} else if(keys[SDLK_KP3] || keys[SDLK_RIGHT] && keys[SDLK_DOWN]) {
		dir = 4;
	} else if(keys[SDLK_KP8] || keys[SDLK_UP]) {
		dir = 0;
	} else if(keys[SDLK_KP2] || keys[SDLK_DOWN]) {
		dir = 3;
	}

	if(dir != -1) {
		using namespace hex;
		dir = (dir + static_cast<int>(orientation))%6;
		return static_cast<DIRECTION>(dir);
	} else {
		return hex::NULL_DIRECTION;
	}
}

bool run()
{
	const Uint8* keys = SDL_GetKeyState(NULL);
	return keys[SDLK_LSHIFT] || keys[SDLK_RSHIFT];
}

bool pass()
{
	const Uint8* keys = SDL_GetKeyState(NULL);
	return keys[SDLK_SPACE];
}
		
}
