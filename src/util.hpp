
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef UTIL_HPP_INCLUDED
#define UTIL_HPP_INCLUDED

namespace util
{

template<typename T>
bool is_odd(T n) { return ((n > 0 ? n : -n)%2) == 1; }

template<typename T>
bool is_even(T n) { return !is_odd(n); }

}

#endif
