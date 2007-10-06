
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

namespace {
#ifdef HAVE_CONFIG_H
  const std::string data_dir=DATADIR ;
  const bool have_datadir = true;
#else
  const std::string data_dir="";
  const bool have_datadir = false;
#endif
}

bool do_file_exists(const std::string& fname)
{
	std::ifstream file(fname.c_str(), std::ios_base::binary);
	if(file.rdstate() != 0) {
		return false;
	}

	file.close();
	return true;
}
	
std::string find_file(const std::string& fname) 
{
	if(do_file_exists(fname)) {
		return fname;
	} 
	if(have_datadir) {
		std::string data_fname = data_dir + "/" + fname;
		if(do_file_exists(data_fname)) {
			return data_fname;
		}
	}
	return fname;
}

bool file_exists(const std::string& name)
{
	return do_file_exists(find_file(name));
}
	
std::string read_file(const std::string& name)
{
	std::string fname = find_file(name);
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
