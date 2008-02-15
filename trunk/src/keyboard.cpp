
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
#include <iostream>

namespace keyboard
{

controls global_controls;

controls::controls() : input::delegate_listener(&keys_) {
    keys_.bind_key(UP_LEFT, SDLK_KP7, KMOD_NONE);
    keys_.bind_key(UP_RIGHT, SDLK_KP9, KMOD_NONE);
    keys_.bind_key(DOWN_LEFT, SDLK_KP1, KMOD_NONE);
    keys_.bind_key(DOWN_RIGHT, SDLK_KP3, KMOD_NONE);
    keys_.bind_key(UP, SDLK_KP8, KMOD_NONE);
    keys_.bind_key(DOWN, SDLK_KP2, KMOD_NONE);
    
    keys_.bind_key(UP, SDLK_UP, KMOD_NONE);
    keys_.bind_key(DOWN, SDLK_DOWN, KMOD_NONE);
    keys_.bind_key(LEFT, SDLK_LEFT, KMOD_NONE);
    keys_.bind_key(RIGHT, SDLK_RIGHT, KMOD_NONE);

    keys_.bind_key(RUN_UP_LEFT, SDLK_KP7, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_UP_RIGHT, SDLK_KP9, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_DOWN_LEFT, SDLK_KP1, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_DOWN_RIGHT, SDLK_KP3, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_UP, SDLK_KP8, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_DOWN, SDLK_KP2, (SDLMod)KMOD_SHIFT);
    
    keys_.bind_key(RUN_UP, SDLK_UP, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_DOWN, SDLK_DOWN, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_LEFT, SDLK_LEFT, (SDLMod)KMOD_SHIFT);
    keys_.bind_key(RUN_RIGHT, SDLK_RIGHT, (SDLMod)KMOD_SHIFT);

    keys_.bind_key(PASS, SDLK_SPACE, KMOD_NONE);
    keys_.bind_key(PASS, SDLK_SPACE, (SDLMod)KMOD_SHIFT);
}
        
    
hex::DIRECTION controls::dir(hex::DIRECTION orientation, 
                             bool& run, bool& pass)
{
    int dir = NONE;
    run = false;

    if(keys_.key(UP_LEFT) || keys_.key(RUN_UP_LEFT)) {
        dir = UP_LEFT;
        run = keys_.key(RUN_UP_LEFT);
    } else if(keys_.key(UP_RIGHT) || keys_.key(RUN_UP_RIGHT)) {
        dir = UP_RIGHT;
        run = keys_.key(RUN_UP_RIGHT);
    } else if(keys_.key(DOWN_LEFT) || keys_.key(RUN_DOWN_LEFT)){
        dir = DOWN_LEFT;
        run = keys_.key(RUN_DOWN_LEFT);
    } else if(keys_.key(DOWN_RIGHT) || keys_.key(RUN_DOWN_RIGHT)) {
        dir = DOWN_RIGHT;
        run = keys_.key(RUN_DOWN_RIGHT);
    } else if(keys_.key(UP) || keys_.key(RUN_UP)) {
        if(keys_.key(LEFT) || keys_.key(RUN_LEFT)) {
            dir = UP_LEFT;
        } else if(keys_.key(RIGHT) || keys_.key(RUN_RIGHT)) {
            dir = UP_RIGHT;
        } else {
            dir = UP;
        }
        run = keys_.key(RUN_UP);
    } else if(keys_.key(DOWN) || keys_.key(RUN_DOWN)) {
        if(keys_.key(LEFT) || keys_.key(RUN_LEFT)) {
            dir = DOWN_LEFT;
        } else if(keys_.key(RIGHT) || keys_.key(RUN_RIGHT)) {
            dir = DOWN_RIGHT;
        } else {
            dir = DOWN;
        }
        run = keys_.key(RUN_DOWN);
    } else if(keys_.key(PASS)) {
        dir = PASS;
    }

    pass = (dir == PASS);
    
	if(dir != NONE && dir != PASS) {
		using namespace hex;
		dir = (dir + static_cast<int>(orientation))%6;
		return static_cast<DIRECTION>(dir);
	} else {
		return hex::NULL_DIRECTION;
	}
}

}
