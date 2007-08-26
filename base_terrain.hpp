
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef BASE_TERRAIN_HPP_INCLUDED
#define BASE_TERRAIN_HPP_INCLUDED

#include <string>
#include <vector>

#include "model_fwd.hpp"
#include "tile_logic.hpp"
#include "wml_node_fwd.hpp"

#include "base_terrain_fwd.hpp"

#include "texture.hpp"

namespace hex
{

class base_terrain
{
public:
	static const_base_terrain_ptr get(const std::string& id);
	static void get_terrain_ids(std::vector<std::string>& res);

	graphics::texture generate_texture(const location& loc, int height, const const_base_terrain_ptr* adj) const;
	graphics::texture transition_texture(hex::DIRECTION dir) const;
	void get_cliff_textures(std::vector<graphics::texture>& result) const;
	wml::const_node_ptr get_cliff_particles() const { return cliff_particles_; }
	graphics::const_model_ptr generate_model(const location& loc,
	           int height) const;
	GLfloat get_rotation(const location& loc, int height) const;

	static void add_terrain(wml::const_node_ptr node);

	const std::string& id() const { return id_; }
	const std::string& name() const { return name_; }

	GLfloat vision_block() const { return vision_block_; }

	int default_cost() const { return default_cost_; }

	const std::string& battle_style() const { return battle_style_; }

private:
	explicit base_terrain(wml::const_node_ptr node);

	std::string id_;
	std::string name_;
	std::vector<std::string> textures_;
	std::vector<std::string> models_;
	std::vector<std::string> cliff_;
	wml::const_node_ptr cliff_particles_;
	int overlap_priority_;
	GLfloat vision_block_;
	int default_cost_;
	std::string battle_style_;
};
		
}

#endif
