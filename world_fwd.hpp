#ifndef WORLD_FWD_HPP_INCLUDED
#define WORLD_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

namespace game_logic {

class world;
typedef boost::intrusive_ptr<world> world_ptr;
typedef boost::intrusive_ptr<const world> const_world_ptr;

}

#endif
