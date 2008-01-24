#ifndef PARTICLE_EMITTER_HPP_INCLUDED
#define PARTICLE_EMITTER_HPP_INCLUDED

#include <GL/glew.h>

#include "formula.hpp"
#include "particle_emitter_fwd.hpp"
#include "particle_system.hpp"
#include "wml_node_fwd.hpp"

namespace graphics {

class particle;
class particle_system;

class particle_emitter {
public:
	particle_emitter(wml::const_node_ptr node,
	                const GLfloat* dir1, const GLfloat* dir2,
	                const GLfloat* pos1, const GLfloat* pos2=0);
	void emit_particle(particle_system& sys);
	void set_pos(const GLfloat* pos);
private:
	void initialize_particle(particle& p) const;
	GLfloat pos1_[3];
	GLfloat pos2_[3];
	GLfloat dir1_[3];
	GLfloat dir2_[3];
	game_logic::formula_ptr pos_diffs_[3];
	game_logic::formula_ptr velocity_diffs_[3];
	game_logic::formula_ptr acceleration_[3];
	game_logic::formula_ptr time_;
	game_logic::formula_ptr size_;
	game_logic::formula_ptr color_[4];
	game_logic::formula_ptr color_diff_[4];
	game_logic::formula_ptr ttl_;
	game_logic::formula_ptr speed_;
	int next_;
};

}

#endif
