#include <iostream>
#include <string>

#include "preferences.hpp"

namespace {

bool nocombat = false;
bool maxfps = false;

bool parse_arg(const std::string& arg)
{
	if(arg == "--nocombat") {
		nocombat = true;
	} else if(arg == "--maxfps") {
		maxfps = true;
	} else {
		std::cerr << "unrecognized argument: '" << arg << "'\n";
		return false;
	}

	return true;
}

}

bool preference_nocombat()
{
	return nocombat;
}

bool preference_maxfps()
{
	return maxfps;
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
