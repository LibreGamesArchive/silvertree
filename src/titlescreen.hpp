#ifndef TITLESCREEN_HPP_INCLUDED
#define TITLESCREEN_HPP_INCLUDED

#include <string>
#include "world.hpp"

namespace title {

void show(game_logic::world_ptr w, const std::string& logo, 
          const std::string& music, int fade_time);

}

#endif
