
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "base_terrain.hpp"
#include "camera.hpp"
#include "foreach.hpp"
#include "material.hpp"
#include "model.hpp"
#include "string_utils.hpp"
#include "texture.hpp"
#include "terrain_feature.hpp"
#include "tile.hpp"

#include <algorithm>
#include <iostream>
#include <math.h>
#include <stack>

#ifdef PROTOTYPE_FRUSTUM_CULLING_ENABLED
#include "frustum.hpp"
#endif

namespace hex
{

namespace {

unsigned int current_id = 1;
unsigned int frame_number = 0;

#ifdef PROTOTYPE_FRUSTUM_CULLING_ENABLED
frustum pc_frustum;
#endif

}

tile::tile(const location& loc, const std::string& data)
	: id_(current_id++), loc_(loc), height_(0), model_init_(false),
	  active_tracker_(NULL)
{

	std::vector<std::string> items = util::split(data,' ');
	if(items.size() >= 2) {
		init(atoi(items[0].c_str()),
		     base_terrain::get(items[1]),
			 items.size() > 2 ? terrain_feature::get(items[2]) :
			  const_terrain_feature_ptr());
	} else {
		init(0,const_base_terrain_ptr(),const_terrain_feature_ptr());
	}
}

tile::tile(const location& loc, int height,
           const_base_terrain_ptr terrain,
		   const_terrain_feature_ptr feature)
	: loc_(loc), height_(height), active_tracker_(NULL)
{
	init(height,terrain,feature);
}

void tile::set_terrain(const std::string& name)
{
	terrain_ = base_terrain::get(name);
	texture_ = graphics::texture();
}

void tile::set_feature(const std::string& name)
{
	feature_ = terrain_feature::get(name);
	model_init_ = false;
	model_ = graphics::const_model_ptr();
}

void tile::init(int height, const_base_terrain_ptr terrain,
                const_terrain_feature_ptr feature)
{
	std::fill(neighbours_,neighbours_+
					sizeof(neighbours_)/sizeof(*neighbours_),
					static_cast<const tile*>(NULL));
	std::fill(cliffs_,cliffs_+
					sizeof(cliffs_)/sizeof(*cliffs_),
					static_cast<const tile*>(NULL));

	height_ = height;
	terrain_ = terrain;
	feature_ = feature;

	center_.position.x() = translate_x(loc_);
	center_.position.y() = translate_y(loc_);
	center_.position.z() = translate_height(height_);
	center_.init = false;
}

bool tile::is_passable(DIRECTION dir) const
{
	if(!neighbours_[dir] || cliffs_[dir]) {
		return false;
	}

	if(neighbours_[dir]->cliffs_[(dir+3)%6]) {
		return false;
	}

	return true;
}

void tile::invalidate()
{
	texture_ = graphics::texture();
	center_.position.x() = translate_x(loc_);
	center_.position.y() = translate_y(loc_);
	center_.position.z() = translate_height(height_);
	center_.init = false;
	for(int n = 0; n != 6; ++n) {
		corners_[n].init = false;
	}
}

void tile::init_corners()
{
	for(int n = 0; n != 6; ++n) {
		if(corners_[n].init == false) {
			calculate_corner(n);
		}
	}

	if(center_.init == false) {

		center_.init = true;
	}
}

GLfloat tile::height_at_point(GLfloat x, GLfloat y) const
{
	Eigen::Vector2f the_point(x,y);
	int index = -1;
	if(y < 0.5) {
		if(x < 0.5) {
			index = 5;
		} else {
			index = 0;
		}
	} else {
		if(x < 0.5) {
			index = 3;
		} else {
			index = 2;
		}
	}

	const point& corner = corners_[index];
	Eigen::Vector2f corner_pos = Eigen::Vector2f(corner.position.x(), corner.position.y());
	const GLfloat dist_corner = (corner_pos - the_point).norm();
	Eigen::Vector2f center_pos = Eigen::Vector2f(center_.position.x(), center_.position.y());
	const GLfloat dist_center = (center_pos - the_point).norm();
	return (dist_corner*center_.position.z() + dist_center*corner.position.z())/(dist_corner+dist_center);
}

GLfloat tile::height_at_point_vision(GLfloat x, GLfloat y) const
{
	return center_.position.z() + (feature_ ? feature_->vision_block() : 0);
}

void tile::init_normals()
{
	Eigen::Vector3f positions[6];
	for(int i = 0; i < 6; i++) {
		positions[i] = corners_[i].position;
	}
	center_.normal.loadZero();
	for(int n = 6; n < 12; n++) {
		Eigen::Vector3f& normal = corners_[n%6].normal;
		normal = (positions[n%6] - positions[(n-1)%6]).cross(positions[(n+1)%6] - positions[n%6]);
		normal.normalize();
		center_.normal += normal;
	}
	center_.normal /= 6;
	center_.normal.normalize();
}

void tile::init_particles()
{
	emitters_.clear();
	if(!terrain_) {
		return;
	}

	for(int n = 0; n != 6; ++n) {
		if(cliffs_[n]) {
			wml::const_node_ptr particle = terrain_->get_cliff_particles();
			if(particle) {
				const point& c1 = corners_[(n+5)%6];
				const point& c2 = corners_[(n+6)%6];
				const Eigen::Vector3f& pos1 = c1.position;
				const Eigen::Vector3f& pos2 = c2.position;
				Eigen::Vector3f dir1 = pos1 - center_.position; dir1.z() = 0.0;
				Eigen::Vector3f dir2 = pos2 - center_.position; dir2.z() = 0.0;
				emitters_.push_back(graphics::particle_emitter(particle, dir1.array(), dir2.array(),  pos1.array(), pos2.array()));
			}
		}
	}

	if(feature_) {
		wml::const_node_ptr particle = feature_->particle_emitter();
		if(particle) {
			const GLfloat null_dir[3] = {0.0,0.0,0.0};
			emitters_.push_back(graphics::particle_emitter(particle, null_dir, null_dir, center_.position.array(), center_.position.array()));
		}
	}
}

void tile::calculate_corner(int n)
{
	const DIRECTION neighbour_a = static_cast<DIRECTION>(n);
	const DIRECTION neighbour_b = static_cast<DIRECTION>((n+1)%6);

	const int point_a = (n+2)%6;
	const int point_b = (n+4)%6;

	const tile* adja = neighbours_[neighbour_a];
	const tile* adjb = neighbours_[neighbour_b];

	if(adja != NULL && adja->corners_[point_a].init) {
		corners_[n] = adja->corners_[point_a];
		return;
	}

	if(adjb != NULL && adjb->corners_[point_b].init) {
		corners_[n] = adjb->corners_[point_b];
		return;
	}

	location adj[6];
	get_adjacent_tiles(loc_,adj);

	corners_[n].position.x() = (translate_x(loc_) +
	                            translate_x(adj[neighbour_a]) +
	                            translate_x(adj[neighbour_b]))/3.0;
	corners_[n].position.y() = (translate_y(loc_) +
	                            translate_y(adj[neighbour_a]) +
	                            translate_y(adj[neighbour_b]))/3.0;

	int sum = height_;
	int num = 1;
	if(adja != NULL) {
		sum += adja->height_;
		++num;
	}

	if(adjb != NULL) {
		sum += adjb->height_;
		++num;
	}

	sum /= num;
	corners_[n].position.z() = translate_height(sum);
	corners_[n].init = true;
}

namespace {

const GLfloat UVVerticalMargin = 10.0/128.0;
const GLfloat UVHorizontalMargin = 33.0/128.0;
const GLfloat UVCenter[] = {0.5,0.5};
const GLfloat UVCorners[6][2] = {
	{1.0-UVHorizontalMargin,1.0-UVVerticalMargin},
	{1.0,0.5},
	{1.0-UVHorizontalMargin,UVVerticalMargin + 2.0/128.0},
	{UVHorizontalMargin + 6.0/128.0,UVVerticalMargin + 2.0/128.0},
	{6.0/128.0,0.5},
	{UVHorizontalMargin + 6.0/128.0,1.0-UVVerticalMargin}
};

}

void tile::setup_drawing()
{
	GLfloat diffuse[] = {0.6,0.6,0.6,1.0};
	GLfloat ambient[] = {0.4,0.4,0.4,1.0};
	GLfloat specular[] = {0.0,0.0,0.0,1.0};
	GLfloat emission[] = {0.0,0.0,0.0,1.0};

	glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
	glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
	glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
	glMaterialfv(GL_FRONT,GL_EMISSION,emission);

#ifdef PROTOTYPE_FRUSTUM_CULLING_ENABLED
	frustum::initialize();
	/* use the below format to find the total area seen by the camera */
	//pc_frustum.set_volume_clip_space(-0.1, 0.1, -0.1, 0.1, -1, 1);
	/* the number is a pushback radius that prevents talin himself,
	   and the things directly underneath him from falling into the
	   frustum; this was the best heuristic i could find */
	pc_frustum.set_volume_world_space(0.75);
#endif

	frame_number++;
}

void tile::finish_drawing()
{
}

void tile::set_cliff(DIRECTION dir, const tile* neighbour) {
	cliffs_[dir] = neighbour;
}

void tile::load_texture() const
{
	if(!texture_.valid() && terrain_) {
		const_base_terrain_ptr adj[6];
		for(int n = 0; n != 6; ++n) {
			if(neighbours_[n] != NULL) {
				adj[n] = neighbours_[n]->terrain_;
			}
		}

		texture_ = terrain_->generate_texture(loc_,height_,adj);
	}
}

void tile::draw(bool own_texture) const
{
    if(own_texture) {
	load_texture();
	texture_.set_as_current_texture();
    }

#ifdef PROTOTYPE_FRUSTUM_CULLING_ENABLED
	bool translucent = pc_frustum.intersects(*this);
	if(translucent) {
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glDepthMask(GL_FALSE);
		glBegin(GL_TRIANGLE_FAN);
		glColor4ub(255, 255, 255, 64);
		draw_point(center_);
		for(int i=0;i<7;++i) {
			const int n = i%6;
			draw_point(corners_[n]);
		}
		glEnd();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_LIGHTING);
		glDepthMask(GL_TRUE);
	} else {
#endif
		glBegin(GL_TRIANGLE_FAN);

		texture_.set_coord(UVCenter[0],UVCenter[1]);

		draw_point(center_);

		for(int i = 0; i != 7; ++i) {
			const int n = i%6;
			texture_.set_coord(UVCorners[n][0],UVCorners[n][1]);
			draw_point(corners_[n]);
		}

		glEnd();
#ifdef PROTOTYPE_FRUSTUM_CULLING_ENABLED
	}
#endif

	if(active_tracker_) {
		active_tracker_->update();
	}
}

void tile::draw_model() const
{
	if(!feature_) {
		return;
	}

	if(!model_init_) {
		model_ = feature_->generate_model(loc_,height_);
		model_init_ = true;
	}

	if(!model_) {
		return;
	}

	glPushMatrix();
	glTranslatef(center_.position.x(), center_.position.y(), center_.position.z());
	glRotatef(feature_->get_rotation(loc_,height_),0.0,0.0,1.0);
	model_->draw();
	glPopMatrix();
}

void tile::draw_center() const
{
	draw_point(center_);
}

void tile::emit_particles(graphics::particle_system& system) const
{
	foreach(graphics::particle_emitter& emitter, emitters_) {
		emitter.emit_particle(system);
	}
}

void tile::adjust_height(int n)
{
	height_ += n;
}

void tile::draw_point(const point& p) const
{
	glNormal3fv(p.normal.array());
	glVertex3fv(p.position.array());
}

namespace {

void cliff_normal(int n, GLfloat& normalx, GLfloat& normaly)
{
	while(n < 0) {
		n += 6;
	}
	switch(n%6) {
	case 0:
		normalx += 0.0;
		normaly += 1.0;
		break;
	case 1:
		normalx += 0.6666;
		normaly += 0.3333;
		break;
	case 2:
		normalx += 0.6666;
		normaly += -0.3333;
		break;
	case 3:
		normalx += 0.0;
		normaly += -1.0;
		break;
	case 4:
		normalx += -0.6666;
		normaly += -0.3333;
		break;
	case 5:
		normalx += -0.6666;
		normaly += 0.3333;
		break;
	default:
		assert(false);
	}
}

}

void tile::draw_cliffs() const
{
	const hex::camera* cam = camera::current_camera();
	if(!cam) {
		return;
	}

	if(!terrain_) {
		return;
	}

	if(cliff_textures_.empty()) {
		terrain_->get_cliff_textures(cliff_textures_);
	}

	graphics::texture& cliff_texture = cliff_textures_[frame_number%cliff_textures_.size()];

	cliff_texture.set_as_current_texture();

	for(int n = 0; n < 6; ++n) {
		if(!cliffs_[n]) {
			continue;
		}

		const int next = (n+1)%6;
		const int prev = n == 0 ? 5 : n-1;

		const point *points[4] = { &cliffs_[n]->corners_[(n+3)%6],
		                           &cliffs_[n]->corners_[(n+2)%6],
				                   &corners_[(n)%6], &corners_[n == 0 ? 5 : n-1]};
		const GLfloat sixth = 1.0/6.0;
		const GLfloat left = sixth*GLfloat(loc_.x()*2 + loc_.y()*2 + n);
		const GLfloat right = left + sixth;
		glBegin(GL_QUADS);
		for(int m = 0; m != 4; ++m) {
			const bool is_left = m == 0 || m == 3;
			const GLfloat u = is_left ? left : right;
			const GLfloat v = points[m]->position.z() * sixth;
			glTexCoord2f(u,v);

			GLfloat normalx = 0.0, normaly = 0.0;
			cliff_normal(n,normalx,normaly);
			cliff_normal(is_left ? prev : next,normalx,normaly);
			glNormal3f(normalx,normaly,0.0);
			glVertex3fv(points[m]->position.array());
		}
		glEnd();

		if(neighbours_[next] && neighbours_[next]->neighbours_[prev]) {
			glNormal3f(0.6666*next,-1.0+0.6666*next,0.0);
			const Eigen::Vector3f& corner = neighbours_[next]->corners_[prev].position;
			glBegin(GL_TRIANGLES);
			glTexCoord2f(right+sixth,corner.z() * sixth);
			glVertex3fv(corner.array());
			glTexCoord2f(right,points[2]->position.z() * sixth);
			glVertex3fv(points[2]->position.array());
			glTexCoord2f(right,points[1]->position.z() * sixth);
			glVertex3fv(points[1]->position.array());
			glEnd();
		}

		if(neighbours_[prev] && neighbours_[prev]->neighbours_[next]) {
			glNormal3f(0.6666*prev,-1.0+0.6666*prev,0.0);
			const Eigen::Vector3f& corner = corners_[prev == 0 ? 5 : prev-1].position;
			glBegin(GL_TRIANGLES);
			glTexCoord2f(left - sixth,corner.z() * sixth);
			glVertex3fv(corner.array());
			glTexCoord2f(left,points[0]->position.z() * sixth);
			glVertex3fv(points[0]->position.array());
			glTexCoord2f(left,points[3]->position.z() * sixth);
			glVertex3fv(points[3]->position.array());
			glEnd();
		}
	}
}

/*
void tile::draw_cliff_transitions() const
{
	const hex::camera* cam = camera::current_camera();
	if(!cam) {
		return;
	}

	if(!terrain_) {
		return;
	}

	graphics::texture overlap_texture = terrain_->transition_texture(hex::NORTH);
	overlap_texture.set_as_current_texture();
	for(int n = 0; n < 6; ++n) {
		if(!cliffs_[n]) {
			continue;
		}

		const point *points[4] = { &cliffs_[n]->corners_[(n+3)%6],
		                           &cliffs_[n]->corners_[(n+2)%6],
				                   &corners_[(n)%6], &corners_[n == 0 ? 5 : n-1]};
		glBegin(GL_TRIANGLES);
		GLfloat normalx = 0.0, normaly = 0.0;
		cliff_normal(n,normalx,normaly);
		overlap_texture.set_coord(UVCenter[0],UVCenter[1]);
		glNormal3f(normalx,normaly,0.0);
		glVertex3f((points[0]->x+points[1]->x)/2.0,
		           (points[0]->y+points[1]->y)/2.0, points[1]->height-1.0);
		overlap_texture.set_coord(UVCorners[0][0],UVCorners[0][1]);
		glNormal3f(normalx,normaly,-1.0);
		glVertex3f(points[1]->x, points[1]->y, points[1]->height);
		overlap_texture.set_coord(UVCorners[5][0],UVCorners[5][1]);
		glNormal3f(normalx,normaly,-1.0);
		glVertex3f(points[0]->x, points[0]->y, points[0]->height);
		glEnd();
	}
}
*/

void tile::draw_grid() const
{
	glBegin(GL_LINE_LOOP);
	glColor4f(0.0,0.0,0.0,1.0);
	for(int n = 0; n != 6; ++n) {
		glVertex3fv((corners_[n].position + Eigen::Vector3f(0.0,0.0,0.1)).array());
	}
	glEnd();
}

void tile::draw_highlight() const
{
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glBegin(GL_TRIANGLE_FAN);

	glColor4f(1.0,1.0,1.0,0.2);

	draw_point(center_);

	for(int i = 0; i != 7; ++i) {
		const int n = i%6;
		draw_point(corners_[n]);
	}

	glEnd();
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_LIGHTING);
}

void tile::attach_tracker(graphics::location_tracker* tracker) const {
	active_tracker_ = tracker;
	active_tracker_->clear_vertices();
	active_tracker_->add_vertex(center_.position.x(), center_.position.y(), center_.position.z());
	for(int i=0; i<6;i++) {
		active_tracker_->add_vertex(corners_[i].position.x(), corners_[i].position.y(), corners_[i].position.z());
	}
}

void tile::clear_tracker() const {
	active_tracker_ = NULL;
}

int tile::neighbour_cliffs(const tile** neighbours) const
{
	int index = 0;
	for(int n = 0; n != 6; ++n) {
		if(cliffs_[n]) {
			neighbours[index++] = cliffs_[n];
		}
	}

	return index;
}

}
