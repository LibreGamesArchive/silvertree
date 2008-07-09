#ifndef BATTLE_MISSILE_HPP_INCLUDED
#define BATTLE_MISSILE_HPP_INCLUDED

#include <GL/glew.h>

#include "model_fwd.hpp"
#include "map_avatar.hpp"

namespace game_logic {

class battle_missile: public hex::basic_drawable {
public:
	battle_missile(graphics::const_model_ptr model,
	               const GLfloat* src,
				   const GLfloat* dst);
	void update();
    hex::const_map_avatar_ptr avatar() const { return avatar_; }
    void update_position(int key) const;
    void update_rotation(int key) const;
private:
    hex::const_map_avatar_ptr avatar_;
	GLfloat src_[3];
	GLfloat dst_[3];
	GLfloat rotate_;
};
}

#endif
