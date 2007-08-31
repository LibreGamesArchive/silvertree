
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "surface_cache.hpp"
#include "texture.hpp"
#include <gl.h>
#include <map>
#include <iostream>

namespace graphics
{

namespace {
	typedef std::map<texture::key,graphics::texture> texture_map;
	texture_map texture_cache;
	const size_t TextureBufSize = 128;
	GLuint texture_buf[TextureBufSize];
	size_t texture_buf_pos = TextureBufSize;
	std::vector<GLuint> avail_textures;

	GLuint current_texture = 0;

	GLuint get_texture_id() {
		if(!avail_textures.empty()) {
			const GLuint res = avail_textures.back();
			avail_textures.pop_back();
			return res;
		}

		if(texture_buf_pos == TextureBufSize) {
			glGenTextures(TextureBufSize, texture_buf);
			texture_buf_pos = 0;
		}

		return texture_buf[texture_buf_pos++];
	}

	bool npot_allowed = true;
	GLfloat width_multiplier = -1.0;
	GLfloat height_multiplier = -1.0;

	unsigned int next_power_of_2(unsigned int n)
	{
		--n;
		n = n|(n >> 1);
		n = n|(n >> 2);
		n = n|(n >> 4);
		n = n|(n >> 8);
		n = n|(n >> 16);
		++n;
		return n;
	}
}

void texture::clear_textures()
{
	texture_cache.clear();
}

texture::texture(const key& surfs)
   : width_(0), height_(0), ratio_w_(1.0), ratio_h_(1.0)
{
	if(surfs.empty() ||
	   std::find(surfs.begin(),surfs.end(),surface()) != surfs.end()) {
		return;
	}

	if(width_multiplier < 0.0) {
		const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
		if(strstr(vendor, "NVIDIA Corporation")) {
			std::cerr << "NVIDIA video card: using npot textures\n";
			npot_allowed = true;
		} else {
			std::cerr << "non-NVIDIA video card: using only pot textures\n";
			npot_allowed = false;
		}

		width_multiplier = 1.0;
		height_multiplier = 1.0;
	}

	width_ = surfs.front()->w;
	height_ = surfs.front()->h;

	unsigned int surf_width = width_;
	unsigned int surf_height = height_;
	if(!npot_allowed) {
		surf_width = surf_height =
		   std::max(next_power_of_2(surf_width),
		            next_power_of_2(surf_height));
		ratio_w_ = GLfloat(width_)/GLfloat(surf_width);
		ratio_h_ = GLfloat(height_)/GLfloat(surf_height);
	}


	surface s(SDL_CreateRGBSurface(SDL_SWSURFACE,surf_width,surf_height,32,SURFACE_MASK));

	for(key::const_iterator i = surfs.begin(); i != surfs.end(); ++i){
		if(i == surfs.begin()) {
			SDL_SetAlpha(i->get(), 0, SDL_ALPHA_OPAQUE);
		} else {
			SDL_SetAlpha(i->get(), SDL_SRCALPHA, SDL_ALPHA_OPAQUE);
		}

		SDL_BlitSurface(i->get(),NULL,s.get(),NULL);
	}

	id_ = boost::shared_ptr<ID>(new ID(get_texture_id()));

	glBindTexture(GL_TEXTURE_2D,id_->id);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D,0,4,s->w,s->h,0,GL_RGBA,
	             GL_UNSIGNED_BYTE,s->pixels);
	current_texture = 0;
}

void texture::set_as_current_texture() const
{
	if(!valid()) {
		glBindTexture(GL_TEXTURE_2D,0);
		current_texture = 0;
		return;
	}

	if(current_texture == id_->id) {
		return;
	}

	current_texture = id_->id;

	glBindTexture(GL_TEXTURE_2D,id_->id);
	width_multiplier = ratio_w_;
	height_multiplier = ratio_h_;
}

texture texture::get(const std::string& str)
{
	return get(surface_cache::get(str));
}

texture texture::get(const key& surfs)
{
	texture_map::iterator i = texture_cache.find(surfs);
	if(i != texture_cache.end()) {
		return i->second;
	} else {

		texture t(surfs);

		texture_cache.insert(std::pair<key,texture>(surfs,t));
		return t;
	}
}

texture texture::get(const surface& surf)
{
	return get(key(1,surf));
}

texture texture::get_no_cache(const key& surfs)
{
	return texture(surfs);
}

texture texture::get_no_cache(const surface& surf)
{
	return texture(key(1,surf));
}

void texture::set_current_texture(const key& k)
{
	texture t(get(k));
	t.set_as_current_texture();
}

void texture::set_coord(GLfloat x, GLfloat y)
{
	if(npot_allowed) {
		glTexCoord2f(x,y);
	} else {
		glTexCoord2f(x*width_multiplier,y*height_multiplier);
	}
}

texture::ID::~ID()
{
	avail_textures.push_back(id);
}

}
