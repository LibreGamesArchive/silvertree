#include "SDL.h"

#include "camera.hpp"
#include "camera_controller.hpp"

namespace hex
{

namespace {
GLfloat MouseLookSpeed = 0.2; // Multiplier for tilting with the mouse.
GLfloat MouseRotateDistance = 50; // Threshold for rotating with the mouse.
}

camera_controller::camera_controller(camera& cam) 
    : cam_(cam), keyboard_pan_(false), 
      wheelup_clicks_(this), wheeldown_clicks_(this), drags_(this)
{
    keys_.bind_key(ROTATE_LEFT, SDLK_COMMA, KMOD_NONE);
    keys_.bind_key(ROTATE_LEFT, SDLK_KP4, KMOD_NONE);
    keys_.bind_key(ROTATE_LEFT, SDLK_LEFT, (SDLMod)KMOD_CTRL);
    keys_.bind_key(ROTATE_RIGHT, SDLK_PERIOD, KMOD_NONE);
    keys_.bind_key(ROTATE_RIGHT, SDLK_KP6, KMOD_NONE);
    keys_.bind_key(ROTATE_RIGHT, SDLK_RIGHT, (SDLMod)KMOD_CTRL);

    keys_.bind_key(TILT_UP, SDLK_p, KMOD_NONE);
    keys_.bind_key(TILT_UP, SDLK_UP, (SDLMod)KMOD_CTRL);
    keys_.bind_key(TILT_DOWN, SDLK_l, KMOD_NONE);
    keys_.bind_key(TILT_DOWN, SDLK_DOWN, (SDLMod)KMOD_CTRL);

    keys_.bind_key(ZOOM_IN, SDLK_z, KMOD_NONE);
    keys_.bind_key(ZOOM_OUT, SDLK_x, KMOD_NONE);

    keys_.bind_key(DEBUG_ADJUST_ON, SDLK_w, KMOD_NONE);
    keys_.bind_key(DEBUG_ADJUST_OFF, SDLK_e, KMOD_NONE);

    keys_.bind_key(PAN_UP, SDLK_UP, (SDLMod)KMOD_ALT);
    keys_.bind_key(PAN_DOWN, SDLK_DOWN, (SDLMod)KMOD_ALT);
    keys_.bind_key(PAN_LEFT, SDLK_LEFT, (SDLMod)KMOD_ALT);
    keys_.bind_key(PAN_RIGHT, SDLK_RIGHT, (SDLMod)KMOD_ALT);

    drags_.set_target_state(SDL_BUTTON(SDL_BUTTON_RIGHT), 
                            input::mouse_listener::STATE_MASK_NONE);
    drags_.set_target_mod(KMOD_NONE, input::mouse_listener::MOD_MASK_NONE);

    wheelup_clicks_.set_target_state(SDL_BUTTON(SDL_BUTTON_WHEELUP), ~SDL_BUTTON_RIGHT);
    wheelup_clicks_.set_target_mod(KMOD_NONE, input::mouse_listener::MOD_MASK_NONE);

    wheeldown_clicks_.set_target_state(SDL_BUTTON(SDL_BUTTON_WHEELDOWN), ~SDL_BUTTON_RIGHT);
    wheeldown_clicks_.set_target_mod(KMOD_NONE, input::mouse_listener::MOD_MASK_NONE);
}

bool camera_controller::process_event(const SDL_Event &e, bool claimed) {
    claimed |= keys_.process_event(e, claimed);
    claimed |= wheelup_clicks_.process_event(e, claimed);
    claimed |= wheeldown_clicks_.process_event(e, claimed);
    claimed |= drags_.process_event(e, claimed);
    return claimed;
}

void camera_controller::click(Sint32 x, Sint32 y, int count, 
                              Uint8 state, Uint8 bstate, SDLMod mod) {
    // Allow zooming with the mouse wheel.
    if(bstate & SDL_BUTTON(SDL_BUTTON_WHEELUP)) {
        cam_.zoom_out();
    }
    if(bstate & SDL_BUTTON(SDL_BUTTON_WHEELDOWN)) {
        cam_.zoom_in();
    }
        
}
void camera_controller::drag() {
    int middle_y = SDL_GetVideoSurface()->h / 2;

    // Allow tilting by holding the right mouse button.
    if (drags_.rel_y() > 0) {
        cam_.tilt_up(drags_.rel_y() * MouseLookSpeed);
    }
    if (drags_.rel_y() < 0) {
        cam_.tilt_down(-drags_.rel_y() * MouseLookSpeed);
    }

    // Allow rotating by holding the right mouse button.
    // We rotate only after the mouse is moved quite far,
    // and the direction depends on if the cursor is in the
    // upper or lower screen half.
    if (drags_.pos_x() > drags_.start_pos_x() + MouseRotateDistance) {
        if (drags_.pos_y() > middle_y) {
            cam_.rotate_right();
        } else {
            cam_.rotate_left();
        }
        drags_.reset_start_pos();
    }
    if (drags_.pos_x() < drags_.start_pos_x() - MouseRotateDistance) {
        if (drags_.pos_y() > middle_y) {
            cam_.rotate_left();
        } else {
            cam_.rotate_right();
        }
        drags_.reset_start_pos();
    }
}

void camera_controller::update() {
    if(keys_.key(ROTATE_LEFT)) {
        cam_.rotate_left();
    }
    if(keys_.key(ROTATE_RIGHT)) {
        cam_.rotate_right();
    }
    if(keys_.key(TILT_UP)) {
        cam_.tilt_up();
    }
    if(keys_.key(TILT_DOWN)) {
        cam_.tilt_down();
    }
    if(keys_.key(ZOOM_IN)) {
        cam_.zoom_in();
    }
    if(keys_.key(ZOOM_OUT)) {
		cam_.zoom_out();
	}
    if(keys_.key(DEBUG_ADJUST_ON)) {
		cam_.set_debug_adjust(true);
	}
    if(keys_.key(DEBUG_ADJUST_OFF)) {
		cam_.set_debug_adjust(false);
	}
	if(!keyboard_pan_) {
        return;
    }
    if(keys_.key(PAN_UP)) {
        cam_.pan_up();
    }
    if(keys_.key(PAN_DOWN)) {
        cam_.pan_down();
    }
    if(keys_.key(PAN_LEFT)) {
        cam_.pan_left();
    }
    if(keys_.key(PAN_RIGHT)) {
        cam_.pan_right();
    }
}

void camera_controller::prepare_selection()
{
	int mousex, mousey;
	SDL_GetMouseState(&mousex, &mousey);
	return cam_.prepare_selection(mousex,mousey);
}

unsigned int camera_controller::finish_selection()
{
	return cam_.finish_selection();
}

}
