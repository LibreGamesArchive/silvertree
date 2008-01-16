#ifndef PARTICLE_HPP_INCLUDED
#define PARTICLE_HPP_INCLUDED

#include <GL/gl.h>

namespace graphics {

class particle {
public:
	friend class particle_emitter;
	void draw();
	bool dead() const { return ttl_ <= 0; }
private:
	GLfloat size_;
	GLfloat pos_[3];
	GLfloat velocity_[3];
	GLfloat acceleration_[3];
	GLfloat color_[4];
	GLfloat color_diff_[4];
	int ttl_;
};

}

#endif
