
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef TILE_HPP_INCLUDED
#define TILE_HPP_INCLUDED

#include <string>
#include <vector>

#include <gl.h>

#include "base_terrain_fwd.hpp"
#include "material.hpp"
#include "model_fwd.hpp"
#include "particle_emitter.hpp"
#include "terrain_feature_fwd.hpp"
#include "texture.hpp"
#include "tile_logic.hpp"

namespace hex
{

class tile
{
public:
	typedef std::vector<graphics::const_material_ptr> features_cache;
	static void initialize_features_cache(const tile** beg, const tile** end,
	                                      features_cache* cache);
	static void draw_features(const tile** beg, const tile** end,
	                          const features_cache& cache);

	tile(const location& loc, const std::string& data);
	tile(const location& loc, int height,
	     const_base_terrain_ptr terrain,
		 const_terrain_feature_ptr feature);

	const location& loc() const { return loc_; }
	int height() const { return height_; }

	void set_neighbour(DIRECTION dir, const tile* neighbour) {
		neighbours_[dir] = neighbour;
	}

	void set_cliff(DIRECTION dir, const tile* neighbour);

	static void setup_drawing();
	static void finish_drawing();
	void draw() const;
	void draw_cliffs() const;
	void draw_model() const;
	void draw_grid() const;
	void draw_highlight() const;

	static GLfloat translate_x(const location& loc) {
		const GLfloat XRatio = 0.86602540378443427;
		return static_cast<GLfloat>(loc.x())*XRatio;
	}

	static GLfloat translate_y(const location& loc) {
		return static_cast<GLfloat>(loc.y()) +
		                ((loc.x()%2) == 0 ? 0.0 : 0.5);
	}

	static GLfloat translate_height(int height) {
		return static_cast<GLfloat>(height)/3.0;
	}

	const_base_terrain_ptr terrain() const { return terrain_; }
	const_terrain_feature_ptr feature() const { return feature_; }

	void init_normals();
	void init_corners();
	void init_particles();

	GLfloat height_at_point(GLfloat x, GLfloat y) const;
	GLfloat height_at_point_vision(GLfloat x, GLfloat y) const;

	void draw_center() const;

	void emit_particles(graphics::particle_system& system) const;

	void adjust_height(int n);
	void set_terrain(const std::string& name);
	void set_feature(const std::string& name);

	bool is_passable(DIRECTION dir) const;

	void invalidate();

	int num_neighbours() const { int res = 0; for(int n = 0; n != 6; ++n) { if(neighbours_[n]) { ++res; } } return res; }

	struct compare_texture
	{
		bool operator()(const tile* a, const tile* b) const {
			return a->texture_ < b->texture_;
		}
	};
	
private:
	void do_draw() const;
	void init(int height, const_base_terrain_ptr terrain,
	          const_terrain_feature_ptr feature);

	GLuint id_;
	location loc_;
	int height_;

	const tile* neighbours_[6];
	const tile* cliffs_[6];
	const_base_terrain_ptr terrain_;
	const_terrain_feature_ptr feature_;
	mutable graphics::const_model_ptr model_;
	mutable bool model_init_;

	mutable graphics::texture texture_;
	mutable std::vector<graphics::texture> cliff_textures_;

	struct point {
		point() : x(0), y(0), height(0),
			  red(1), green(1), blue(1), init(false)
		{
		}
		
		GLfloat x, y, height;
		GLfloat red, green, blue;
		GLfloat normal[3];
		bool init;
	};

	point center_;
	point corners_[6];

	mutable std::vector<graphics::particle_emitter> emitters_;

	void calculate_corner(int n);

	void draw_point(const point& p) const;
};

}

#endif
