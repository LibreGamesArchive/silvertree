
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef SETTLEMENT_FWD_HPP_INCLUDED
#define SETTLEMENT_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace game_logic
{
class settlement;
typedef boost::shared_ptr<settlement> settlement_ptr;
typedef boost::shared_ptr<const settlement> const_settlement_ptr;
}

#endif
