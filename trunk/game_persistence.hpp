#ifndef GAME_PERSISTENCE_HPP_INCLUDED
#define GAME_PERSISTENCE_HPP_INCLUDED

namespace game_dialogs {

void silent_save(const std::string& filename);
bool save(const std::string& filename, game_logic::world *wp);

}

#endif
