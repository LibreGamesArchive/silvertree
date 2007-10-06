#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

#include <GL/gl.h>

#include <string>

bool parse_args(int argc, char** argv);

bool preference_nocombat();
bool preference_maxfps();
bool preference_mipmapping();

GLenum preference_mipmap_min();
GLenum preference_mipmap_max();

const std::string& preference_save_file();

#endif
