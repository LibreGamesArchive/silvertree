#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

#include <gl.h>

bool parse_args(int argc, char** argv);

bool preference_nocombat();
bool preference_maxfps();
bool preference_mipmapping();

GLenum preference_mipmap_min();
GLenum preference_mipmap_max();

#endif
