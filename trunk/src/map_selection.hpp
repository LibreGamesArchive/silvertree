#ifndef MAP_SELECTION_HPP_INCLUDED
#define MAP_SELECTION_HPP_INCLUDED

#include <SDL.h>

#include "input.hpp"
#include "renderer.hpp"
#include "tile_logic.hpp"

namespace input {

class map_selection: public listener {
public:
    explicit map_selection(graphics::renderer& renderer);
    bool process_event(const SDL_Event& e, bool claimed);
    void reset();
    int get_selected_avatar();
    const hex::location& get_selected_hex();
private:
    graphics::renderer& renderer_;
    hex::location selected_hex_;
    int selected_avatar_;
    bool hex_up_to_date_;
    bool avatar_up_to_date_;
};

typedef boost::shared_ptr<map_selection> map_selection_ptr;
typedef boost::shared_ptr<const map_selection> const_map_selection_ptr;

}

#endif
