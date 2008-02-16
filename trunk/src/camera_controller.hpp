#ifndef CAMERA_CONTROLLER_HPP_INCLUDED
#define CAMERA_CONTROLLER_HPP_INCLUDED

#include "input.hpp"

namespace hex
{

class camera;

class camera_controller: public input::listener
{
public:
    enum {
        ROTATE_LEFT = 0,
        ROTATE_RIGHT,
        TILT_UP,
        TILT_DOWN,
        ZOOM_IN,
        ZOOM_OUT,
        DEBUG_ADJUST_ON,
        DEBUG_ADJUST_OFF,
        PAN_UP,
        PAN_DOWN,
        PAN_LEFT,
        PAN_RIGHT
    };
    
    explicit camera_controller(camera& cam);
    void allow_keyboard_panning(bool value=true) {
        keyboard_pan_ = value;
    }

    bool process_event(const SDL_Event& e, bool claimed);
    void reset();

    void update();
    void click(Sint32 x, Sint32 y, int count, 
               Uint8 state, Uint8 bstate, SDLMod mod);
    void drag();

    void prepare_selection();
    unsigned int finish_selection();

private:
    camera& cam_;
	bool keyboard_pan_;
	GLfloat drag_x_, drag_y_;
    input::key_down_listener keys_;
    input::mouse_click_listener<camera_controller*> wheelup_clicks_, wheeldown_clicks_;
    input::active_mouse_drag_listener<camera_controller*> drags_;
};

typedef boost::shared_ptr<camera_controller> camera_controller_ptr;

}

#endif
