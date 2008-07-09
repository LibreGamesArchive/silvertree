#include <iostream>

#include "particle.hpp"

namespace graphics {

void particle::draw()
{
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color_);
    glVertex3f(pos_[0]-size_, pos_[1], pos_[2]);
    glVertex3f(pos_[0]+size_, pos_[1], pos_[2]);
    glVertex3f(pos_[0], pos_[1]+size_, pos_[2]);
    for(int n = 0; n != 3; ++n) {
        pos_[n] += velocity_[n];
        velocity_[n] += acceleration_[n];
    }
    
    for(int n = 0; n != 4; ++n) {
        color_[n] += color_diff_[n];
    }
    ttl_--;
}

}
