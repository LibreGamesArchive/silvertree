#include <iostream>
#include <string>

#include <boost/lexical_cast.hpp>

#include <SDL.h>

#include "filesystem.hpp"
#include "preferences.hpp"
#include "string_utils.hpp"

namespace {

bool nocombat = false;
bool maxfps = false;
bool sliders = false;

bool mipmapping = true;
GLenum mipmap_min = GL_NEAREST_MIPMAP_LINEAR, mipmap_max = GL_LINEAR;
int screen_width = 1024;
int screen_height = 768;
bool fullscreen = false;
std::string save_file;
std::string scenario_file = "data/scenario.cfg";

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

bool parse_arg(const std::string& arg)
{
	if(util::string_starts_with(arg, "--width=")) {
		std::string rest = util::strip_string_prefix(arg, "--width=");
		screen_width = boost::lexical_cast<int>(rest);
	} else if(util::string_starts_with(arg, "--height=")) {
		std::string rest = util::strip_string_prefix(arg, "--height=");
		screen_height = boost::lexical_cast<int>(rest);
	} else if(arg == "--fullscreen") {
		fullscreen = true;
	} else if(arg == "--windowed") {
		fullscreen = false;
	} else if(arg == "--nocombat") {
		nocombat = true;
	} else if(arg == "--maxfps") {
		maxfps = true;
	} else if(arg == "--sliders") {
		sliders = true;
	} else if(arg == "--disable-mipmapping") {
		mipmapping = false;
	} else if(util::string_starts_with(arg, "--mipmapmin")) {
		std::string rest = util::strip_string_prefix(arg, "--mipmapmin");
		if(rest == "" || rest=="=" || rest.substr(0,1) != "=") {
			std::cerr << "format: mipmapmin={n,l,nn,nl,ln,ll}\n";
			return false;
		} else {
			mipmap_min = mipmap_arg_to_type(rest.substr(1));
		}
	} else if(util::string_starts_with(arg, "--mipmapmax")) {
		std::string rest = util::strip_string_prefix(arg, "--mipmapmax");
		if(rest == "" || rest == "=" || rest.substr(0,1) != "=") {
			std::cerr << "format: mipmapmax={n,l,nn,nl,ln,ll}\n";
			return false;
		} else {
			mipmap_max = mipmap_arg_to_type(rest.substr(1));
		}
	} else if(util::string_starts_with(arg, "--save=")) {
		save_file = sys::get_saves_dir() + "/" + util::strip_string_prefix(arg, "--save=");
	} else if(util::string_starts_with(arg, "--scenario=")) {
		scenario_file = util::strip_string_prefix(arg, "--scenario=");
	} else if(arg == "-h" || arg == "--help") {
		std::cout << "usage:\n"
			<< "  --fullscreen                   start the game in full screen mode.\n"
			<< "  --windowed                     start the game in windowed mode.\n"
			<< "  --height=<Y>                   set the screen height to 'Y'.\n"
			<< "  --width=<X>                    set the screen width to 'X'.\n"
			<< "  --maxfps                       run the game at the maximum fps possible.\n"
			<< "  --mipmapmin={n,l,nn,nl,ln,ll}  \n"
			<< "  --mipmapmax={n,l,nn,nl,ln,ll}  \n"
			<< "  --disable-mipmapping           disable mipmapping.\n"
			<< "  --nocombat                     debug mode where enemies don't initiate an attack.\n"
			<< "  --save=<savefile>              load the save 'savefile'.\n"
			<< "  --scenario=<scenario.cfg>      start the game with the given scenario file.\n"
			<< "  -h, --help                     print this message and exits.\n"
			;
		return false;
	} else {
		std::cerr << "unrecognized argument: '" << arg << "'\n";
		return false;
	}

	return true;
}

}

bool preference_sliders() {
	return sliders;
}

bool preference_mipmapping() {
	return mipmapping;
}

GLenum preference_mipmap_min() {
	return mipmap_min;
}

GLenum preference_mipmap_max() {
	return mipmap_max;
}

bool preference_nocombat()
{
	return nocombat;
}

bool preference_maxfps()
{
	return maxfps;
}

int preference_screen_width()
{
	return screen_width;
}

int preference_screen_height()
{
	return screen_height;
}

unsigned int preference_fullscreen()
{
	return fullscreen ? SDL_FULLSCREEN : 0;
}

const std::string& preference_save_file()
{
	return save_file;
}

const std::string& preference_scenario_file()
{
	return scenario_file;
}

bool parse_args(int argc, char** argv)
{
	for(int n = 1; n < argc; ++n) {
		if(!parse_arg(argv[n])) {
			return false;
		}
	}

	return true;
}

