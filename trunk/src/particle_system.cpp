#include "camera.hpp"
#include "particle_system.hpp"

namespace graphics {

namespace {
const int MaxParticles = 1024*8;
}

particle& particle_system::add()
{
    while(!dead_.empty()) {
        const int index = dead_.back();
        dead_.pop_back();
        if(index < particles_.size()) {
            return particles_[index];
        }
    }
    
    particles_.push_back(particle());
    return particles_.back();
}

void particle_system::draw()
{

    //glPushAttrib(GL_ALL_ATTRIB_BITS);
    GLfloat diffuse[] = {0.0,0.0,0.0,0.0};
    GLfloat specular[] = {0.0,0.0,0.0,0.0};
    
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular);
    
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    int last_alive = -1;
    for(int n = 0; n != particles_.size(); ++n) {
        if(particles_[n].dead()) {
            continue;
        }
        
        particles_[n].draw();
        if(particles_[n].dead()) {
            dead_.push_back(n);
        } else {
            last_alive = n;
        }
    }
    
    glEnd();
    glEnable(GL_TEXTURE_2D);
    
    particles_.resize(last_alive+1);
    //glPopAttrib();
}

}
