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
using std::cerr;
using boost::program_options::options_description;
using boost::program_options::variables_map;
using boost::program_options::store;
using boost::program_options::value;
using boost::program_options::validation_error;
using boost::program_options::validators::check_first_occurrence;
using boost::program_options::validators::get_single_string;

namespace {

variables_map options;

GLenum mipmap_arg_to_type(const std::string& arg, bool minification) {
	if(arg == "l") {
		return GL_LINEAR;
	} else if(arg == "n") {
		return GL_NEAREST;
	}
	if(minification) {
		if(arg == "nn") {
			return GL_NEAREST_MIPMAP_NEAREST;
		} else if(arg == "ll") {
			return GL_LINEAR_MIPMAP_LINEAR;
		} else if(arg == "ln") {
			return GL_LINEAR_MIPMAP_NEAREST;
		} else if(arg == "nl") {
			return GL_NEAREST_MIPMAP_LINEAR;
		}
	}
	throw validation_error("incorrect texture filter setting.");
}

}

template <bool minification> struct filtering_setting
{
	filtering_setting(GLenum setting) : filter(setting) {}
	GLenum filter;
};

template <bool minification> void validate(boost::any& v, 
              const std::vector<std::string>& values,
              filtering_setting<minification>* target_type, int)
{
	check_first_occurrence(v);
	const string& arg = get_single_string(values);

	v = boost::any(filtering_setting<minification>(mipmap_arg_to_type(arg, minification)));
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
		("texture-filter-min", value<filtering_setting<true> >(), "Texture minification function(n,l,nn,nl,ln,ll).")
		("texture-filter-mag", value<filtering_setting<false> >(), "Texture magnification function(n,l).")
	;

	options_description all("Allowed options");
	all.add(generic).add(general).add(graphics);

	try {
		store(parse_command_line(argc, argv, all), options);
	} catch(boost::program_options::error& e) {
		cerr << "Error in command-line: " << e.what() << std::endl;
		return false;
	}

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
	return options.count("texture-filter-min") ? options["texture-filter-min"].as<filtering_setting<true> >().filter : GL_NEAREST;
}

GLenum preference_mipmap_max() {
	return options.count("texture-filter-mag") ? options["texture-filter-mag"].as<filtering_setting<false> >().filter : GL_NEAREST;
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
