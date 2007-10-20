#ifndef PARTICLE_SYSTEM_HPP_INCLUDED
#define PARTICLE_SYSTEM_HPP_INCLUDED

#include <vector>

#include "particle.hpp"

namespace graphics {

class particle_system {
public:
	particle& add();
	void draw();
private:
	std::vector<particle> particles_;
	std::vector<int> dead_;
};

}

#endif
