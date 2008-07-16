#include "map_selection.hpp"

namespace input {

map_selection::map_selection(graphics::renderer& renderer) 
    :  renderer_(renderer),
       selected_hex_(-1,-1),
       selected_avatar_(-1),
       hex_up_to_date_(true),
       avatar_up_to_date_(true)
{}

int map_selection::get_selected_avatar() {
    if(!avatar_up_to_date_) {
        selected_avatar_ = renderer_.get_selected_avatar();
        avatar_up_to_date_ = true;
    }
    return selected_avatar_;
}

const hex::location& map_selection::get_selected_hex() {
    if(!hex_up_to_date_) {
        std::cout << "Asked renderer for selected_hex\n";
        selected_hex_ = renderer_.get_selected_hex();
        hex_up_to_date_ = true;
    }
    return selected_hex_;
}

bool map_selection::process_event(const SDL_Event& e, bool claimed) {
    switch(e.type) {
    case SDL_MOUSEMOTION:
        if(!claimed) {
            hex_up_to_date_ = false;
            avatar_up_to_date_ = false;
            claimed = true;
        } else {
            reset();
        }
        break;
    case SDL_ACTIVEEVENT:
        if((e.active.gain == 0) && (e.active.state & SDL_APPMOUSEFOCUS)) {
            // even if wasn't claimed it is now
            // either way, need to clear the selection
            reset();
            claimed = true;
        }
        break;
    }
    return claimed;
}

void map_selection::reset() {
    hex_up_to_date_ = true;
    avatar_up_to_date_ = true;
    selected_hex_ = hex::location(-1,-1);
    selected_avatar_ = -1;
}

}
