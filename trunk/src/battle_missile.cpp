#include <cmath>
#include <numeric>
#include <string.h>

#include <iostream>

#include "battle_missile.hpp"
#include "foreach.hpp"
#include "model.hpp"

namespace game_logic {

namespace {
const GLfloat PI = 3.14159265;
}

battle_missile::battle_missile(graphics::const_model_ptr model,
                               const GLfloat* src,
                               const GLfloat* dst)
  : avatar_(hex::map_avatar::create(model, graphics::surface(), this)), 
    rotate_(0.0)
{
	memcpy(src_, src, sizeof(src_));
	memcpy(dst_, dst, sizeof(dst_));

	const GLfloat xdiff = dst_[0] - src_[0];
	const GLfloat ydiff = dst_[1] - src_[1];

	if(ydiff == 0.0) {
		rotate_ = 0.0;
	} else {
		rotate_ = atan(ydiff/xdiff)*180.0/PI;
	}
}

void battle_missile::update()
{
	if(!avatar_) {
		return;
	}

	const GLfloat speed = 0.5;
	GLfloat diff[] = { 
        dst_[0] - src_[0], 
        dst_[1] - src_[1], 
        dst_[2] - src_[2] 
    };

	GLfloat sum = 0;
	foreach(const GLfloat& num, diff) {
		sum += std::abs(num);
	}

	if(sum < speed) {
		avatar_.reset();
		return;
	}

	foreach(GLfloat& num, diff) {
		num *= speed/sum;
	}

	for(int n = 0; n != 3; ++n) {
		src_[n] += diff[n];
	}
}

void battle_missile::update_position(int key) const {
    position_s(0) = src_[0];
    position_s(1) = src_[1];
    position_s(2) = src_[2];
}
void battle_missile::update_rotation(int key) const {
    rotation_s(0) = rotate_;
}

}
