
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "filesystem.hpp"

#include <fstream>
#include <sstream>

namespace sys
{

std::string read_file(const std::string& fname)
{
	std::ifstream file(fname.c_str(),std::ios_base::binary);
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

void write_file(const std::string& fname, const std::string& data)
{
	std::ofstream file(fname.c_str(),std::ios_base::binary);
	file << data;
}
		
}
