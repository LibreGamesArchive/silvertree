#ifndef CAMERA_CONTROLLER_HPP_INCLUDED
#define CAMERA_CONTROLLER_HPP_INCLUDED

namespace hex
{

class camera;

class camera_controller
{
public:
	explicit camera_controller(camera& cam) : cam_(cam), keyboard_pan_(false)
	{}

	void allow_keyboard_panning(bool value=true) {
		keyboard_pan_ = value;
	}

	void keyboard_control();
	void prepare_selection();
	unsigned int finish_selection();

private:
	camera& cam_;
	bool keyboard_pan_;
};

}

#endif
