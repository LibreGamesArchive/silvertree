#include <iostream>
#include <string>

#include "preferences.hpp"

namespace {

bool nocombat = false;

bool parse_arg(const std::string& arg)
{
	if(arg == "--nocombat") {
		nocombat = true;
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

bool parse_args(int argc, char** argv)
{
	for(int n = 1; n < argc; ++n) {
		if(!parse_arg(argv[n])) {
			return false;
		}
	}

	return true;
}
