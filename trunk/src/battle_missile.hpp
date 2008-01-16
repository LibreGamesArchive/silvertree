#ifndef BATTLE_MISSILE_HPP_INCLUDED
#define BATTLE_MISSILE_HPP_INCLUDED

#include <GL/gl.h>

#include "model_fwd.hpp"

namespace game_logic {

class battle_missile {
public:
	battle_missile(graphics::const_model_ptr model,
	               const GLfloat* src,
				   const GLfloat* dst);
	void update();
	void draw();
private:
	graphics::const_model_ptr model_;
	GLfloat src_[3];
	GLfloat dst_[3];
	GLfloat rotate_;
};
}

#endif
