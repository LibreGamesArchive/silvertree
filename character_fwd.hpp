#ifndef CHARACTER_FWD_HPP_INCLUDED
#define CHARACTER_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

namespace game_logic {

class character;
typedef boost::intrusive_ptr<character> character_ptr;
typedef boost::intrusive_ptr<const character> const_character_ptr;

}

#endif
