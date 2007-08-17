
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

namespace hex
{

namespace {

unsigned int current_id = 1;
const unsigned int num_display_lists = 10001;
unsigned int display_list_map[num_display_lists];
GLuint start_display_list = GLuint(-1);
int display_hit = 0;
int display_miss = 0;

GLuint get_display_list(unsigned int tile_id, bool* is_new)
{
        if(start_display_list == GLuint(-1)) {
                start_display_list = glGenLists(num_display_lists);
        }

        const unsigned int index = tile_id%num_display_lists;
        *is_new = display_list_map[index] != tile_id;
        if(*is_new) {
                display_list_map[index] = tile_id;
                display_miss++;
        } else {
                display_hit++;
        }

        if(((display_miss+display_hit)%10000) == 0) {
                std::cerr << "hits: " << display_hit << " misses: " << display_miss << "\n";
        }

        return start_display_list + index;
}

void invalidate_display_list(unsigned int tile_id)
{
        const unsigned int index = tile_id%num_display_lists;
        if(display_list_map[index] == tile_id) {
                display_list_map[index] = 0;
        }
}
}

void tile::initialize_features_cache(const tile** beg, const tile** end,
                                     tile::features_cache* cache)
{
	cache->clear();
	for(; beg != end; ++beg) {
		const tile& t = **beg;
		if(!t.feature_) {
			continue;
		}

		if(t.model_init_ == false) {
			t.model_ = t.feature_->generate_model(t.loc_,t.height_);
		}

		if(!t.model_) {
			continue;
		}

		t.model_->get_materials(cache);
	}

	std::cerr << "got " << cache->size() << " materials\n";
}

void tile::draw_features(const tile** beg, const tile** end,
                         const tile::features_cache& cache)
{
	foreach(const graphics::const_material_ptr& mat, cache) {
		if(mat) {
			mat->set_as_current_material();
		}
		for(const tile** i = beg; i != end; ++i) {
			const tile& t = **i;
			if(t.model_) {
				glPushMatrix();
				glTranslatef(t.center_.x,t.center_.y,t.center_.height);
				glRotatef(t.feature_->get_rotation(t.loc_,t.height_),0.0,0.0,1.0);
				t.model_->draw_material(mat);
				glPopMatrix();
			}
		}
	}
}

tile::tile(const location& loc, const std::string& data)
    : id_(current_id++), loc_(loc), height_(0), model_init_(false)
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
    : loc_(loc), height_(height)
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

	center_.x = translate_x(loc_);
	center_.y = translate_y(loc_);
	center_.height = translate_height(height_);
	center_.init = false;
}

void tile::invalidate()
{
	invalidate_display_list(id_);
	texture_ = graphics::texture();
	center_.x = translate_x(loc_);
	center_.y = translate_y(loc_);
	center_.height = translate_height(height_);
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
		GLfloat vectors[6][3];
		for(int n = 0; n != 6; ++n) {
			vectors[n][0] = corners_[n].x - center_.x;
			vectors[n][1] = corners_[n].y - center_.y;
			vectors[n][2] = corners_[n].height - center_.height;
		}

		std::fill(center_.normal,center_.normal+3,0.0);

		for(int n = 0; n != 6; ++n) {
			const GLfloat* v1 = vectors[n];
			const GLfloat* v2 = vectors[(n+1)%6];
			GLfloat normal[3];
			normal[0] = v1[1]*v2[2] - v1[2]*v2[1];
			normal[1] = v1[2]*v2[0] - v1[0]*v2[2];
			normal[2] = v1[0]*v2[1] - v1[1]*v2[0];

			if(normal[2] > 0.0) {
				normal[0] *= -1.0;
				normal[1] *= -1.0;
				normal[2] *= -1.0;
			}

			center_.normal[0] += normal[0];
			center_.normal[1] += normal[1];
			center_.normal[2] += normal[2];
		}

		center_.init = true;
	}
}

GLfloat tile::height_at_point(GLfloat x, GLfloat y) const
{
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
	const GLfloat dist_corner =
	    sqrt((corner.x-x)*(corner.x-x)+(corner.y-y)*(corner.y-y));
	const GLfloat dist_center =
	    sqrt((center_.x-x)*(center_.x-x)+(center_.y-y)*(center_.y-y));
	return (dist_corner*center_.height + dist_center*corner.height)/(dist_corner+dist_center);
}

GLfloat tile::height_at_point_vision(GLfloat x, GLfloat y) const
{
	return center_.height + (feature_ ? feature_->vision_block() : 0);
}

void tile::init_normals()
{
	for(int n = 0; n != 6; ++n) {
		GLfloat num = 1.0;
		std::copy(center_.normal,center_.normal+3,
		          corners_[n].normal);
		const tile* n1 = neighbours_[n];
		const tile* n2 = neighbours_[(n+1)%6];
		if(n1 != NULL) {

			for(int m = 0; m != 3; ++m) {
				corners_[n].normal[m] += n1->center_.normal[m];
			}

			num += 1.0;
		}

		if(n2 != NULL) {
			for(int m = 0; m != 3; ++m) {
				corners_[n].normal[m] += n2->center_.normal[m];
			}

			num += 1.0;
		}

		for(int m = 0; m != 3; ++m) {
			corners_[n].normal[m] /= num;
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

	corners_[n].x = (translate_x(loc_) +
	                 translate_x(adj[neighbour_a]) +
	                 translate_x(adj[neighbour_b]))/3.0;
	corners_[n].y = (translate_y(loc_) +
	                 translate_y(adj[neighbour_a]) +
	                 translate_y(adj[neighbour_b]))/3.0;

	GLfloat red = center_.red;
	GLfloat green = center_.green;
	GLfloat blue = center_.blue;

	int sum = height_;
	int num = 1;
	if(adja != NULL) {
		sum += adja->height_;
		red += adja->center_.red;
		green += adja->center_.green;
		blue += adja->center_.blue;
		++num;
	}

	if(adjb != NULL) {
		sum += adjb->height_;
		red += adjb->center_.red;
		green += adjb->center_.green;
		blue += adjb->center_.blue;
		++num;
	}

	sum /= num;
	corners_[n].height = translate_height(sum);
	corners_[n].red = red/static_cast<GLfloat>(num);
	corners_[n].green = green/static_cast<GLfloat>(num);
	corners_[n].blue = blue/static_cast<GLfloat>(num);
	corners_[n].init = true;
}

namespace {

const GLfloat UVVerticalMargin = 9.0/125.0;
const GLfloat UVHorizontalMargin = 33.0/125.0;
const GLfloat UVCenter[] = {0.5,0.5};
const GLfloat UVCorners[6][2] = {
	{1.0-UVHorizontalMargin,UVVerticalMargin},
	{1.0,0.5},
	{1.0-UVHorizontalMargin,1.0-UVVerticalMargin},
	{UVHorizontalMargin,1.0-UVVerticalMargin},
	{0.0,0.5},
	{UVHorizontalMargin,UVVerticalMargin} };
		
}

void tile::setup_drawing()
{
	GLfloat diffuse[] = {1.0,1.0,1.0,1.0};
	GLfloat ambient[] = {0.6,0.6,0.6,1.0};
	GLfloat specular[] = {0.0,0.0,0.0,0.0};

	glMaterialfv(GL_FRONT,GL_DIFFUSE,diffuse);
	glMaterialfv(GL_FRONT,GL_AMBIENT,ambient);
	glMaterialfv(GL_FRONT,GL_SPECULAR,specular);
}

void tile::finish_drawing()
{
}

void tile::draw() const
{
	do_draw();
/*
	bool is_new;
	GLuint display_list = get_display_list(id_,&is_new);
	if(is_new) {
		glNewList(display_list,GL_COMPILE_AND_EXECUTE);
		do_draw();
		glEndList();
	} else {
		glCallList(display_list);
	}
*/
}

void tile::do_draw() const
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

	texture_.set_as_current_texture();

	glBegin(GL_TRIANGLE_FAN);

	graphics::texture::set_coord(UVCenter[0],UVCenter[1]);
	
	draw_point(center_);

	for(int i = 0; i != 7; ++i) {
		const int n = i%6;
		graphics::texture::set_coord(UVCorners[n][0],UVCorners[n][1]);
		draw_point(corners_[n]);
	}
	
	glEnd();
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
	glTranslatef(center_.x,center_.y,center_.height);
	glRotatef(feature_->get_rotation(loc_,height_),0.0,0.0,1.0);
	model_->draw();
	glPopMatrix();
}

void tile::draw_center() const
{
	draw_point(center_);
}

void tile::adjust_height(int n)
{
	height_ += n;
}

void tile::draw_point(const point& p) const
{
	glNormal3fv(p.normal);
	glVertex3f(p.x,p.y,p.height);
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
		normaly += -1.0;
		break;
	case 1:
		normalx += -0.6666;
		normaly += -0.3333;
		break;
	case 2:
		normalx += -0.6666;
		normaly += 0.3333;
		break;
	case 3:
		normalx += 0.0;
		normaly += 1.0;
		break;
	case 4:
		normalx += 0.6666;
		normaly += 0.3333;
		break;
	case 5:
		normalx += 0.6666;
		normaly += -0.3333;
		break;
	default:
		assert(false);
	}
}

}

void tile::draw_cliffs() const
{
	if(std::count(cliffs_,cliffs_+6,static_cast<tile*>(NULL)) == 6) {
		return;
	}

	if(!terrain_) {
		return;
	}

	if(cliff_texture_.valid() == false) {
		cliff_texture_ = terrain_->cliff_texture();
	}

	cliff_texture_.set_as_current_texture();
	graphics::texture overlap_texture = terrain_->transition_texture(hex::NORTH);

	for(int n = 0; n != 6; ++n) {
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
			const GLfloat v = points[m]->height * sixth;
			glTexCoord2f(u,v);

			GLfloat normalx = 0.0, normaly = 0.0;
			cliff_normal(n,normalx,normaly);
			cliff_normal(is_left ? prev : next,normalx,normaly);
			glNormal3f(normalx,normaly,0.0);
			glVertex3f(points[m]->x, points[m]->y, points[m]->height);
		}
		glEnd();

		if(neighbours_[next] && neighbours_[next]->neighbours_[prev]) {
			glNormal3f(0.6666*next,-1.0+0.6666*next,0.0);
			const point& corner = neighbours_[next]->corners_[prev];
			glBegin(GL_TRIANGLES);
			glTexCoord2f(right+sixth,corner.height * sixth);
			glVertex3f(corner.x, corner.y, corner.height);
			glTexCoord2f(right,points[2]->height * sixth);
			glVertex3f(points[2]->x, points[2]->y, points[2]->height);
			glTexCoord2f(right,points[1]->height * sixth);
			glVertex3f(points[1]->x, points[1]->y, points[1]->height);
			glEnd();
		}

		if(neighbours_[prev] && neighbours_[prev]->neighbours_[next]) {
			glNormal3f(0.6666*prev,-1.0+0.6666*prev,0.0);
			const point& corner = corners_[prev == 0 ? 5 : prev-1];
			glBegin(GL_TRIANGLES);
			glTexCoord2f(left - sixth,corner.height * sixth);
			glVertex3f(corner.x, corner.y, corner.height);
			glTexCoord2f(left,points[0]->height * sixth);
			glVertex3f(points[0]->x, points[0]->y, points[0]->height);
			glTexCoord2f(left,points[3]->height * sixth);
			glVertex3f(points[3]->x, points[3]->y, points[3]->height);
			glEnd();
		}

		overlap_texture.set_as_current_texture();
		glBegin(GL_TRIANGLES);
		GLfloat normalx = 0.0, normaly = 0.0;
		cliff_normal(n,normalx,normaly);
		graphics::texture::set_coord(UVCenter[0],UVCenter[1]);
		glNormal3f(normalx,normaly,0.0);
		glVertex3f((points[0]->x+points[1]->x)/2.0,
		           (points[0]->y+points[1]->y)/2.0, points[1]->height-1.0);
		graphics::texture::set_coord(UVCorners[0][0],UVCorners[0][1]);
		glNormal3f(normalx,normaly,-1.0);
		glVertex3f(points[1]->x, points[1]->y, points[1]->height);
		graphics::texture::set_coord(UVCorners[5][0],UVCorners[5][1]);
		glNormal3f(normalx,normaly,-1.0);
		glVertex3f(points[0]->x, points[0]->y, points[0]->height);
		glEnd();

		cliff_texture_.set_as_current_texture();
	}
}

void tile::draw_grid() const
{
	glBegin(GL_LINE_LOOP);
	glColor4f(0.0,0.0,0.0,1.0);
	for(int n = 0; n != 6; ++n) {
		glVertex3f(corners_[n].x,corners_[n].y,corners_[n].height+0.1);
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

}
