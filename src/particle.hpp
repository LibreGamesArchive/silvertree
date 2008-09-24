#ifndef PARTICLE_HPP_INCLUDED
#define PARTICLE_HPP_INCLUDED

#include <GL/glew.h>

#include "eigen/vector.h"

#include "animation.hpp"

namespace graphics {

class particle {
public:
	friend class particle_emitter;
	void draw();
	bool dead() const { return position_anim_.finished(); }
private:
	GLfloat size_;
	function_animation<Eigen::Vector3f, quadratic_function<Eigen::Vector3f> > position_anim_;
	Eigen::Vector4f color_;
	Eigen::Vector4f color_diff_;
	int ttl_;
};

}

#endif
