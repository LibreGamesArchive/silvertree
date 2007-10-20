#ifndef PARTICLE_EMITTER_FWD_HPP_INCLUDED
#define PARTICLE_EMITTER_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace graphics {

class particle_emitter;
typedef boost::shared_ptr<particle_emitter> particle_emitter_ptr;
typedef boost::shared_ptr<const particle_emitter> const_particle_emitter_ptr;

}

#endif
