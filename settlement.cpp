
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "gamemap.hpp"
#include "model.hpp"
#include "party.hpp"
#include "settlement.hpp"
#include "tile.hpp"
#include "world.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace game_logic
{

settlement::settlement(const wml::const_node_ptr& node,
                       const hex::gamemap& map)
  : loc_(wml::get_attr<int>(node,"x"),wml::get_attr<int>(node,"y")),
    model_(graphics::model::get_model((*node)["model"])),
	wml_(node), map_(map)
{
}

void settlement::draw() const
{
	using hex::tile;
	if(!map_.is_loc_on_map(loc_) || !model_) {
		return;
	}
	GLfloat pos[3] = {tile::translate_x(loc_), tile::translate_y(loc_),
	         tile::translate_height(map_.get_tile(loc_).height())};
	glPushMatrix();
	glTranslatef(pos[0],pos[1],pos[2]);
	model_->draw();
	glPopMatrix();
}

void settlement::enter(party_ptr pty)
{
	if(!world_) {
		world_.reset(new world(wml_));
	}

	world& w = *world_;
	pty->new_world(w,hex::location(
	         wml::get_attr<int>(wml_,"entry_x"),
	         wml::get_attr<int>(wml_,"entry_y")));
	w.add_party(pty);
	w.play();
}

}
