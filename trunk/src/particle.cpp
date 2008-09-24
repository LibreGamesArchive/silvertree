#include <iostream>

#include "particle.hpp"

namespace graphics {

void particle::draw()
{
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, color_.array());
    Eigen::Vector3f pos = position_anim_.play();
    pos.x() -= size_;
    glVertex3fv(pos.array());
    pos.x() += 2 * size_;
    glVertex3fv(pos.array());
    pos.x() -= size_;
    pos.y() += size_;
    glVertex3fv(pos.array());

    color_ += color_diff_;
    ttl_--;
}

}
