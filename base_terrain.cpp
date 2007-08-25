
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#include <boost/lexical_cast.hpp>

#include "base_terrain.hpp"
#include "model.hpp"
#include "string_utils.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

#include <iostream>

namespace hex
{

namespace {

std::map<std::string,base_terrain_ptr> terrains;
		
}

const_base_terrain_ptr base_terrain::get(const std::string& id)
{
	std::map<std::string,base_terrain_ptr>::const_iterator i =
			terrains.find(id);
	if(i == terrains.end()) {
		return const_base_terrain_ptr();
	} else {
		return i->second;
	}
}

void base_terrain::get_terrain_ids(std::vector<std::string>& res)
{
	for(std::map<std::string,base_terrain_ptr>::const_iterator i =
	    terrains.begin(); i != terrains.end();  ++i) {
		res.push_back(i->first);
	}
}

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

graphics::texture base_terrain::generate_texture(
 const location& loc, int height,const const_base_terrain_ptr* adj) const
{
	if(textures_.empty()) {
		return graphics::texture();
	}
	
	const unsigned int index = prandom(loc,height)%textures_.size();

	graphics::surface surf(
	         graphics::surface_cache::get(textures_[index]));

	graphics::texture::key surfs;
	if(surf) {
		surfs.push_back(surf);
	}

	for(int n = 0; n != 6; ++n) {
		if(adj[n] && adj[n]->overlap_priority_ > overlap_priority_) {
			const std::string name = adj[n]->name_ + "-" +
			   direction_abbreviation(static_cast<DIRECTION>(n))
			   + ".png";
			graphics::surface surf(
			    graphics::surface_cache::get(name));
			if(surf) {
				surfs.push_back(surf);
			}
		}
	}

	return graphics::texture::get(surfs);
}

graphics::texture base_terrain::transition_texture(hex::DIRECTION dir) const
{
	const std::string name = name_ + "-" + direction_abbreviation(dir) + ".png";
	return graphics::texture::get(graphics::surface_cache::get(name));
}

void base_terrain::get_cliff_textures(std::vector<graphics::texture>& result) const
{
	foreach(const std::string& str, cliff_) {
		int ncopies = 1;
		std::string::const_iterator colon = std::find(str.begin(),str.end(),':');
		const std::string image_key(str.begin(),colon);
		if(colon != str.end()) {
			ncopies = boost::lexical_cast<int>(std::string(colon+1,str.end()));
		}
		graphics::texture::key surfs;
		surfs.push_back(graphics::surface_cache::get(image_key));

		while(ncopies-- > 0) {
			result.push_back(graphics::texture::get(surfs));
		}
	}
}

graphics::const_model_ptr base_terrain::generate_model(
				const hex::location& loc, int height) const
{
	if(models_.empty()) {
		return graphics::const_model_ptr();
	}

	const unsigned int index = prandom(loc,height)%models_.size();
	return graphics::model::get_model(models_[index]);
}

GLfloat base_terrain::get_rotation(const location& loc, int height) const
{
	return prandom(loc,height)%360;
}

void base_terrain::add_terrain(wml::const_node_ptr node)
{
	const base_terrain_ptr ptr(new base_terrain(node));
	terrains.insert(std::pair<std::string,base_terrain_ptr>(
							ptr->id_,ptr));
	std::cerr << "loaded terrain '" << ptr->id_ << "'\n";
	                     
}

base_terrain::base_terrain(wml::const_node_ptr node)
    : overlap_priority_(wml::get_attr<int>(node,"overlap_priority")),
      vision_block_(wml::get_attr<GLfloat>(node,"vision_block")),
	  default_cost_(wml::get_int(node,"cost",100)),
	  battle_style_(wml::get_str(node,"battle_style"))
{
	id_ = (*node)["id"];
	name_ = (*node)["name"];
	cliff_ = util::split((*node)["cliff_texture"]);

	textures_ = util::split((*node)["textures"]);
	models_ = util::split((*node)["models"]);
}
		
}
