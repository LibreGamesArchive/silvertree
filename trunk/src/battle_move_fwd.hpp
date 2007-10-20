#ifndef BATTLE_MOVE_FWD_HPP_INCLUDED
#define BATTLE_MOVE_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace game_logic
{
class battle_move;
typedef boost::shared_ptr<battle_move> battle_move_ptr;
typedef boost::shared_ptr<const battle_move> const_battle_move_ptr;
}

#endif
