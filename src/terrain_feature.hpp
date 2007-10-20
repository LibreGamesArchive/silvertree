
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TERRAIN_FEATURE_HPP_INCLUDED
#define TERRAIN_FEATURE_HPP_INCLUDED

#include <GL/gl.h>

#include <string>
#include <vector>

#include "model_fwd.hpp"
#include "terrain_feature_fwd.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

namespace hex
{

class terrain_feature
{
public:
	static const_terrain_feature_ptr get(const std::string& id);
	static void get_feature_ids(std::vector<std::string>& res);
	static void add_terrain(wml::const_node_ptr node);

	explicit terrain_feature(wml::const_node_ptr node);

	graphics::const_model_ptr generate_model(const location& loc,
	                                         int height) const;
	GLfloat get_rotation(const location& loc, int height) const;

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }

	GLfloat vision_block() const { return vision_block_; }

	int default_cost() const { return default_cost_; }

	const wml::const_node_ptr particle_emitter() const { return particle_emitter_; }

private:
	std::string id_;
	std::string name_;
	std::vector<std::string> models_;
	GLfloat vision_block_;
	int default_cost_;
	wml::const_node_ptr particle_emitter_;
};

}

#endif
