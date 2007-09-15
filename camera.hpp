
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef CAMERA_HPP_INCLUDED
#define CAMERA_HPP_INCLUDED

#include <gl.h>
#include <vector>

#include "tile_logic.hpp"

namespace hex
{

class gamemap;

class camera
{
public:
	static camera* current_camera();

	explicit camera(const gamemap& m);
	~camera();

	void prepare_frame();
	void prepare_selection(int x, int y);
	GLuint finish_selection(std::vector<GLuint>* items=NULL);

	void pan_up();
	void pan_down();
	void pan_left();
	void pan_right();

	void set_pan(const GLfloat* buf);
	void set_pan_x(GLfloat x) { translatex_ = x; }
	void set_pan_y(GLfloat y) { translatey_ = y; }
	GLfloat get_pan_x() const { return translatex_; }
	GLfloat get_pan_y() const { return translatey_; }

	void rotate_left();
	void rotate_right();

	void tilt_up();
	void tilt_down();

	void zoom_in();
	void zoom_out();

	DIRECTION direction() const;

	void allow_keyboard_panning(bool value=true) { keyboard_pan_ = value; }
	void keyboard_control();

	GLfloat current_rotation() const { return rotate_; }

	void set_debug_adjust(bool val) { debug_adjust_ = val; }

	void set_dim(int width, int height) { width_ = width; height_ = height; }

	bool is_moving() const;

	const hex::DIRECTION* visible_cliffs(int* num) const;

	void set_background_color(const GLfloat* col);

private:

	GLfloat rotate_radians() const;
	void enforce_limits();
	
	const gamemap& map_;

	int width_, height_;

	GLfloat translatex_, translatey_, translatez_;

	GLfloat rotate_;
	hex::DIRECTION dir_;

	void update_visible_cliffs();
	hex::DIRECTION visible_cliffs_[4];
	int num_visible_cliffs_;

	GLfloat target_rotation() const;
	GLfloat need_to_rotate() const;

	GLfloat tilt_;
	GLfloat zoom_;
	bool debug_adjust_;
	bool keyboard_pan_;

	std::vector<GLuint> selection_;

	GLfloat background_[3];
};
		
}

#endif
