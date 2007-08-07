#ifndef BATTLE_MENU_FWD_HPP_INCLUDED
#define BATTLE_MENU_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace gui {

class battle_menu;
typedef boost::shared_ptr<battle_menu> battle_menu_ptr;
typedef boost::shared_ptr<const battle_menu> const_battle_menu_ptr;

}

#endif
