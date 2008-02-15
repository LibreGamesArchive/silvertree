
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef KEYBOARD_HPP_INCLUDED
#define KEYBOARD_HPP_INCLUDED

#include "input.hpp"
#include "tile_logic.hpp"

namespace keyboard
{
    class controls: public input::delegate_listener {
    public:
        enum {
            NONE = -1,
            UP = 0,
            UP_LEFT = 1,
            DOWN_LEFT = 2,
            DOWN = 3,
            DOWN_RIGHT = 4,
            UP_RIGHT = 5,
            LEFT,
            RIGHT,
            PASS,
            RUN_UP,
            RUN_UP_LEFT,
            RUN_DOWN_LEFT,
            RUN_DOWN,
            RUN_DOWN_RIGHT,
            RUN_UP_RIGHT,
            RUN_LEFT,
            RUN_RIGHT
        };

        controls();
        hex::DIRECTION dir(hex::DIRECTION orientation, 
                           bool& run, bool& pass);
    private:
        input::key_down_listener keys_;
    };

    extern controls global_controls;
}

#endif
