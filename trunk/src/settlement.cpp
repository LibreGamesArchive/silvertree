
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "foreach.hpp"
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
  : model_(graphics::model::get_model((*node)["model"])),
	wml_(node), map_(map)
{
	const std::string& model_height_formula = node->attr("model_height");
	if(model_height_formula.empty() == false) {
		model_height_formula_.reset(new formula(model_height_formula));
	}

	const std::string& model_rotation_formula = node->attr("model_rotation");
	if(model_rotation_formula.empty() == false) {
		model_rotation_formula_.reset(new formula(model_rotation_formula));
	}

	const std::vector<wml::const_node_ptr> portals = wml::child_nodes(node, "portal");
	foreach(const wml::const_node_ptr& portal, portals) {
		hex::location loc1(wml::get_attr<int>(portal,"xdst"),
		                   wml::get_attr<int>(portal,"ydst"));
		hex::location loc2(wml::get_attr<int>(portal,"xsrc"),
		                   wml::get_attr<int>(portal,"ysrc"));
		portals_[loc2] = loc1;
	}
}

wml::node_ptr settlement::write() const
{
	wml::node_ptr res(new wml::node("settlement"));
	wml::node_ptr world_node = get_world().write();
	wml::copy_over(world_node, res);

	if(model_) {
		res->set_attr("model", model_->id());
	}

	if(model_height_formula_) {
		res->set_attr("model_rotation", model_height_formula_->str());
	}

	if(model_rotation_formula_) {
		res->set_attr("model_height", model_rotation_formula_->str());
	}
	
	typedef std::pair<hex::location, hex::location> LocPair;
	foreach(const LocPair& locs, portals_) {
		res->add_child(write_src_dst_location("portal", locs.first, locs.second));
	}

	return res;
}

void settlement::entry_points(std::vector<hex::location>& result) const
{
	typedef std::pair<hex::location,hex::location> loc_pair;
	foreach(const loc_pair& portal, portals_) {
		result.push_back(portal.first);
	}
}

bool settlement::has_entry_point(const hex::location& loc) const
{
	return portals_.count(loc) != 0;
}

namespace {
class real_world_time_callable : public formula_callable
{
	variant get_value(const std::string& key) const {
		if(key == "ms") {
			return variant(SDL_GetTicks());
		} else {
			return variant();
		}
	}

	void get_inputs(std::vector<formula_input>* inputs) const {
		inputs->push_back(formula_input("ms", FORMULA_READ_ONLY));
	}
};
}

void settlement::draw() const
{
	using hex::tile;

	GLfloat height_adjust = 0.0;
	if(model_height_formula_) {
		const int res = model_height_formula_->execute(real_world_time_callable()).as_int();
		height_adjust = res/1000.0;
	}

	typedef std::pair<hex::location,hex::location> loc_pair;
	foreach(const loc_pair& portal, portals_) {
		const hex::location& loc = portal.first;
		if(!map_.is_loc_on_map(loc) || !model_) {
			continue;
		}
		GLfloat pos[3] = {tile::translate_x(loc), tile::translate_y(loc),
		       tile::translate_height(map_.get_tile(loc).height()) + height_adjust};
		glPushMatrix();
		glTranslatef(pos[0],pos[1],pos[2]);

		if(model_rotation_formula_) {
			const variant var = model_rotation_formula_->execute(real_world_time_callable());

			int rotate[4] = {0,0,0,0};
			for(int n = 0; n < 4 && n < var.num_elements(); ++n) {
				rotate[n] = var[n].as_int();
			}

			glRotatef(rotate[0], rotate[1], rotate[2], rotate[3]);
		}
		model_->draw();
		glPopMatrix();
	}
}

game_time settlement::enter(party_ptr pty, const hex::location& loc,
				            const game_time& t)
{
	get_world();

	assert(portals_.count(loc));

	world& w = *world_;
	w.advance_time_until(t);
	const hex::location old_loc = pty->loc();
	pty->new_world(w,portals_[loc]);
	w.add_party(pty);
	w.play();
	if(!pty->loc().valid()) {
		pty->set_loc(old_loc);
	}

	return w.current_time();
}

void settlement::play()
{
	get_world();
	world_->play();
}

const world& settlement::get_world() const
{
	if(!world_) {
		world_.reset(new world(wml_));
	}

	return *world_;
}

}
