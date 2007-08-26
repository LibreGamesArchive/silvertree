
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <map>

#include "model.hpp"
#include "string_utils.hpp"
#include "terrain_feature.hpp"
#include "wml_utils.hpp"

namespace hex
{

namespace {

std::map<std::string,terrain_feature_ptr> terrains;

}

const_terrain_feature_ptr terrain_feature::get(const std::string& id)
{
	std::map<std::string,terrain_feature_ptr>::const_iterator i =
	             terrains.find(id);
	if(i == terrains.end()) {
		return const_terrain_feature_ptr();
	} else {
		return i->second;
	}
}

void terrain_feature::get_feature_ids(std::vector<std::string>& res)
{
	for(std::map<std::string,terrain_feature_ptr>::const_iterator i =
	    terrains.begin(); i != terrains.end(); ++i) {
		res.push_back(i->first);
	}
}

void terrain_feature::add_terrain(wml::const_node_ptr node)
{
	const terrain_feature_ptr ptr(new terrain_feature(node));
	terrains.insert(std::pair<std::string,terrain_feature_ptr>(
	                             ptr->id_,ptr));
}

terrain_feature::terrain_feature(wml::const_node_ptr node)
  : id_(wml::get_str(node,"id")), name_(wml::get_str(node,"name")),
    models_(util::split(wml::get_str(node,"models"))),
    vision_block_(wml::get_attr<GLfloat>(node,"vision_block")),
	default_cost_(wml::get_int(node,"cost", -1))
{}

namespace {
int prandom(const location& loc, int height)
{
	const unsigned int a = (loc.x() + 92872973) ^ 918273;
	const unsigned int b = (loc.y() + 1672517) ^ 128123;
	const unsigned int c = (height + 127390) ^ 13923787;
	const unsigned int random = a*b*c + a*b + b*c + a*c + a + b + c;
	return random;
}
}

graphics::const_model_ptr terrain_feature::generate_model(
				const hex::location& loc, int height) const
{
	if(models_.empty()) {
		return graphics::const_model_ptr();
	}

	const unsigned int index = prandom(loc,height)%models_.size();
	return graphics::model::get_model(models_[index]);
}

GLfloat terrain_feature::get_rotation(const location& loc, int height) const
{
	return prandom(loc,height)%360;
}

}
