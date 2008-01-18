#include "SDL.h"

#include "camera.hpp"
#include "camera_controller.hpp"

namespace hex
{

namespace {
GLfloat MouseLookSpeed = 0.2; // Multiplier for tilting with the mouse.
GLfloat MouseRotateDistance = 50; // Threshold for rotating with the mouse.
}

void camera_controller::keyboard_control()
{
	const Uint8* keys = SDL_GetKeyState(NULL);

	const bool ctrl = keys[SDLK_LCTRL] || keys[SDLK_RCTRL];

	if(keys[SDLK_COMMA] || keys[SDLK_KP4] || ctrl && keys[SDLK_LEFT]) {
		cam_.rotate_left();
	}

	if(keys[SDLK_PERIOD] || keys[SDLK_KP6] || ctrl && keys[SDLK_RIGHT]) {
		cam_.rotate_right();
	}

	if(keys[SDLK_p] || ctrl && keys[SDLK_UP]) {
		cam_.tilt_up();
	}

	if(keys[SDLK_l] || ctrl && keys[SDLK_DOWN]) {
		cam_.tilt_down();
	}

	if(keys[SDLK_z]) {
		cam_.zoom_in();
	}

	if(keys[SDLK_x]) {
		cam_.zoom_out();
	}

	if(keys[SDLK_w]) {
		cam_.set_debug_adjust(true);
	}

	if(keys[SDLK_e]) {
		cam_.set_debug_adjust(false);
	}

	if(!keyboard_pan_) {
		return;
	}

	if(keys[SDLK_UP]) {
		cam_.pan_up();
	}

	if(keys[SDLK_DOWN]) {
		cam_.pan_down();
	}

	if(keys[SDLK_LEFT]) {
		cam_.pan_left();
	}

	if(keys[SDLK_RIGHT]) {
		cam_.pan_right();
	}
}

void camera_controller::event_control(SDL_Event &event)
{
	switch(event.type) {
		case SDL_MOUSEBUTTONDOWN:
			// Allow zooming with the mouse wheel.
			if(event.button.button == SDL_BUTTON_WHEELUP)
				cam_.zoom_out();
			if(event.button.button == SDL_BUTTON_WHEELDOWN)
				cam_.zoom_in();
			if(event.button.button == SDL_BUTTON_MIDDLE) {
				drag_x_ = event.button.x;
				drag_y_ = event.button.y;
			}
			break;
		case SDL_MOUSEMOTION:
			// Allow tilting by holding the middle mouse button.
			if(event.motion.state & SDL_BUTTON(2)) {
				if (event.motion.yrel > 0)
					cam_.tilt_up(event.motion.yrel * MouseLookSpeed);
				if (event.motion.yrel < 0)
					cam_.tilt_down(-event.motion.yrel * MouseLookSpeed);
				if (event.motion.x > drag_x_ + MouseRotateDistance) {
					cam_.rotate_left();
					drag_x_ = event.motion.x;
				}
				if (event.motion.x < drag_x_ - MouseRotateDistance) {
					cam_.rotate_right();
					drag_x_ = event.motion.x;
				}
			}
			break;
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
