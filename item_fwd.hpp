
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef ITEM_FWD_HPP_INCLUDED
#define ITEM_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace game_logic
{

class item;
class equipment;

typedef boost::shared_ptr<item> item_ptr;
typedef boost::shared_ptr<const item> const_item_ptr;
typedef boost::shared_ptr<equipment> equipment_ptr;
typedef boost::shared_ptr<const equipment> const_equipment_ptr;

enum ITEM_TYPE { ITEM_NONE,
                 EQUIPMENT_WEAPON, EQUIPMENT_SHIELD, EQUIPMENT_ARMOR,
                 EQUIPMENT_HELMET };
enum { BEGIN_EQUIPMENT=EQUIPMENT_WEAPON };

}

#endif
