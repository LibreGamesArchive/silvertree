#include <iostream>

#include <string.h>

#include "particle.hpp"
#include "particle_emitter.hpp"
#include "particle_system.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace graphics {

namespace {

void init_formula(game_logic::formula_ptr& f, const wml::const_node_ptr& node,
                  const std::string& str)
{
	if(node->has_attr(str)) {
		f.reset(new game_logic::formula((*node)[str]));
	}
}

}

particle_emitter::particle_emitter(wml::const_node_ptr node,
                                   const GLfloat* dir1, const GLfloat* dir2,
                                   const GLfloat* pos1, const GLfloat* pos2)
{
	if(pos2 == NULL) {
		pos2 = pos1;
	}

	if(dir2 == NULL) {
		dir2 = dir1;
	}

	memcpy(pos1_, pos1, sizeof(pos1_));
	memcpy(pos2_, pos2, sizeof(pos2_));
	memcpy(dir1_, dir1, sizeof(dir1_));
	memcpy(dir2_, dir2, sizeof(dir2_));

	init_formula(pos_diffs_[0], node, "pos_x");
	init_formula(pos_diffs_[1], node, "pos_y");
	init_formula(pos_diffs_[2], node, "pos_z");
	init_formula(velocity_diffs_[0], node, "velocity_x");
	init_formula(velocity_diffs_[1], node, "velocity_y");
	init_formula(velocity_diffs_[2], node, "velocity_z");
	init_formula(acceleration_[0], node, "accel_x");
	init_formula(acceleration_[1], node, "accel_y");
	init_formula(acceleration_[2], node, "accel_z");
	init_formula(color_[0], node, "red");
	init_formula(color_[1], node, "green");
	init_formula(color_[2], node, "blue");
	init_formula(color_[3], node, "alpha");
	init_formula(color_diff_[0], node, "dred");
	init_formula(color_diff_[1], node, "dgreen");
	init_formula(color_diff_[2], node, "dblue");
	init_formula(color_diff_[3], node, "dalpha");

	speed_.reset(new game_logic::formula((*node)["speed"]));
	ttl_.reset(new game_logic::formula((*node)["ttl"]));
	time_.reset(new game_logic::formula((*node)["time"]));
	size_.reset(new game_logic::formula((*node)["size"]));

	next_ = time_->execute().as_int();
}

void particle_emitter::emit_particle(particle_system& system)
{
	next_ -= 100;
	while(next_ <= 0) {
		initialize_particle(system.add());

		const int time = time_->execute().as_int();
		next_ += time;
		if(time <= 0) {
			break;
		}
	}
}

void particle_emitter::set_pos(const GLfloat* pos)
{
	memcpy(pos1_, pos, sizeof(pos1_));
	memcpy(pos2_, pos, sizeof(pos2_));
}

void particle_emitter::initialize_particle(particle& p) const
{
	p.size_ = size_->execute().as_int()/100.0;
	p.ttl_ = ttl_->execute().as_int();
	const GLfloat g = (rand()%1000)/1000.0;
	const GLfloat h = 1.0 - g;
	const GLfloat speed = speed_->execute().as_int()/1000.0;
	Eigen::Vector3f particle_pos;
	Eigen::Vector3f particle_velocity;
	Eigen::Vector3f particle_acceleration;
	for(int n = 0; n != 3; ++n) {
		particle_pos[n] = pos1_[n]*g + pos2_[n]*h;
		if(pos_diffs_[n]) {
			particle_pos[n] += pos_diffs_[n]->execute().as_int()/1000.0;
		}

		particle_velocity[n] = (dir1_[n]*g + dir2_[n]*h)*speed;
		if(velocity_diffs_[n]) {
			particle_velocity[n] += velocity_diffs_[n]->execute().as_int()/45.;
		}

		if(acceleration_[n]) {
			particle_acceleration[n] = acceleration_[n]->execute().as_int()/40.;
		} else {
			particle_acceleration[n] = 0.0;
		}
	}
	p.position_anim_ = function_animation<Eigen::Vector3f, quadratic_function<Eigen::Vector3f> >(quadratic_function<Eigen::Vector3f>(particle_acceleration, particle_velocity, particle_pos));
	p.position_anim_.set_duration((float)p.ttl_/30.);

	for(int n = 0; n != 4; ++n) {
		if(color_[n]) {
			p.color_[n] = color_[n]->execute().as_int()/100.0;
		} else {
			p.color_[n] = 0.0;
		}

		if(color_diff_[n]) {
			p.color_diff_[n] = color_diff_[n]->execute().as_int()/100.0;
		} else {
			p.color_diff_[n] = 0.0;
		}
	}
}

}
