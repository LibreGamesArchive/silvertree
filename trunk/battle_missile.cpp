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
  : model_(model), rotate_(0.0)
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
	if(!model_) {
		return;
	}

	const GLfloat speed = 0.5;
	GLfloat diff[] = { dst_[0] - src_[0], dst_[1] - src_[1], dst_[2] - src_[2] };
	GLfloat sum = 0;
	foreach(const GLfloat& num, diff) {
		sum += std::abs(num);
	}

	if(sum < speed) {
		model_.reset();
		return;
	}

	foreach(GLfloat& num, diff) {
		num *= speed/sum;
	}

	for(int n = 0; n != 3; ++n) {
		src_[n] += diff[n];
	}
}

void battle_missile::draw()
{
	if(!model_) {
		return;
	}

	glPushMatrix();
	glTranslatef(src_[0], src_[1], src_[2]);
	glRotatef(rotate_, 0.0, 0.0, 1.0);
	model_->draw();
	glPopMatrix();
}

}
