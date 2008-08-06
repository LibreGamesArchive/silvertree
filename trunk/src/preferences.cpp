#include <iostream>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include <SDL.h>

#include "filesystem.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"

using std::string;
using std::cout;
using boost::program_options::options_description;
using boost::program_options::variables_map;
using boost::program_options::store;
using boost::program_options::value;

namespace {

variables_map options;

GLenum mipmap_min = GL_NEAREST_MIPMAP_LINEAR, mipmap_max = GL_LINEAR;

GLenum mipmap_arg_to_type(const std::string& arg) {
	if(arg == "l") {
		return GL_LINEAR;
	} else if(arg == "n") {
		return GL_NEAREST;
	} else if(arg == "nn") {
		return GL_NEAREST_MIPMAP_NEAREST;
	} else if(arg == "ll") {
		return GL_LINEAR_MIPMAP_LINEAR;
	} else if(arg == "ln") {
		return GL_LINEAR_MIPMAP_NEAREST;
	} else if(arg == "nl") {
		return GL_NEAREST_MIPMAP_LINEAR;
	}
	std::cerr << "unknown mipmap type \""<<arg<<"\": set to \"n\"\n";
	return GL_NEAREST;
}

}

bool parse_args(int argc, char** argv)
{
	options_description generic;
	generic.add_options()
		("help", "produce a help message")
	;
	options_description general("General options");
	general.add_options()
		("nocombat", "debug mode where enemies don't initiate an attack.")
		("nosliders", "disable sliders in combat.")
		("save", value<string>(), "load the specified saved game.")
		("scenario", value<string>(), "start the game with the given scenario file.")
	;
	options_description graphics("Graphics options");
	graphics.add_options()
		("fullscreen", "start the game in fullscreen mode.")
		("maxfps", "run the game at the maximum fps possible.")
		("width", value<int>(), "set the screen/window width.")
		("height", value<int>(), "set the screen/window height.")
		("disable-mipmapping", "disable mipmapping.")
	;

	options_description all("Allowed options");
	all.add(generic).add(general).add(graphics);

	store(parse_command_line(argc, argv, all), options);

	if(options.count("help")) {
		cout << all;
		return false;
	}
	return true;
}

bool preference_sliders() {
	return !options.count("nosliders");
}

bool preference_mipmapping() {
	return !options.count("disable-mipmapping");
}

GLenum preference_mipmap_min() {
	return GL_NEAREST;
}

GLenum preference_mipmap_max() {
	return GL_NEAREST_MIPMAP_NEAREST;
}

bool preference_nocombat()
{
	return options.count("nocombat");
}

bool preference_maxfps()
{
	return options.count("maxfps");
}

int preference_screen_width()
{
	return options.count("width") ? options["width"].as<int>() : 1024;
}

int preference_screen_height()
{
	return options.count("height") ? options["height"].as<int>() : 768;
}

unsigned int preference_fullscreen()
{
	return options.count("fullscreen") ? SDL_FULLSCREEN : 0;
}

const std::string preference_save_file()
{
	return options.count("save") ? options["save"].as<string>() : string();
}

const std::string preference_scenario_file()
{
	return options.count("scenario") ? options["scenario"].as<string>() : string("data/scenario.cfg");
}
