
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "map_avatar.hpp"
#include "model.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"

#include <GL/glew.h>
#include <iostream>

namespace hex
{

map_avatar_ptr map_avatar::create(wml::const_node_ptr node,
                                  const drawable* owner,
                                  int key)
{
	return map_avatar_ptr(new map_avatar(node,owner,key));
}

map_avatar_ptr map_avatar::create(graphics::const_model_ptr model,
                                  graphics::surface image,
                                  const drawable* owner,
                                  int key) {
    return map_avatar_ptr(new map_avatar(model, image, owner, key));
}

map_avatar::map_avatar(wml::const_node_ptr node, 
                       const drawable* owner,
                       int key)
    : owner_(owner),
      key_(key),
      model_id_((*node)["model"]),
      image_id_((*node)["image"]),
      has_node_(true)
{
    if(model_id_.empty() == false) {
        model_ = graphics::model::get_model((*node)["model"]);
    }
    if(image_id_.empty() == false) {
        image_ = graphics::surface_cache::get((*node)["image"]);
    }
    init();
}

map_avatar::map_avatar(graphics::const_model_ptr model,
                       graphics::surface image,
                       const drawable* owner,
                       int key) 
    : owner_(owner),
      key_(key),
      model_(model),
      image_(image),
      has_node_(false) 
{
    init();
}
    

void map_avatar::init() {
    std::cout<<"map_avatar::init\n";
    loc_tracker_.add_vertex(0.5, 0, 0.0);
    loc_tracker_.add_vertex(-0.5, 0, 0.0);
    loc_tracker_.add_vertex(0.5, 0, 1.0);
    loc_tracker_.add_vertex(-0.5, 0, 1.0);
}

bool map_avatar::valid() const
{
	return model_ || image_;
}

void map_avatar::write(wml::node_ptr node) const
{
    if(has_node_) {
        node->set_attr("model", model_id_);
        node->set_attr("image", image_id_);
    }
}

void map_avatar::draw(bool forSelection) const
{
    if(!valid()) {
        return;
    }

    const drawable::float3f& pos = owner_->position(key_);
    const drawable::float3f& scale = owner_->scale(key_);
    const drawable::float4f& rotate = owner_->rotation(key_);

    glPushMatrix();
    glTranslatef(pos[0],pos[1],pos[2]);
    glRotatef(rotate[0], rotate[1], rotate[2], rotate[3]);
    glScalef(scale[0], scale[1], scale[2]);

    if(model_) {
        model_->draw();
    } else if(image_) {
        if(image_.get()) {
            graphics::texture t(graphics::texture::get(image_));
            t.set_as_current_texture();
            glBegin(GL_QUADS);
            t.set_coord(0.0,0.0);
            glVertex3f(-0.5,0.0,1.0);
            
            t.set_coord(1.0,0.0);
            glVertex3f(0.5,0.0,1.0);
            
            t.set_coord(1.0,1.0);
            glVertex3f(0.5,0.0,0.0);
            
            t.set_coord(0.0,1.0);
            glVertex3f(-0.5,0.0,0.0);
            
            glEnd();
        }
    }
    if(!forSelection) {
        loc_tracker_.update();
    }
    glPopMatrix();
}

}
