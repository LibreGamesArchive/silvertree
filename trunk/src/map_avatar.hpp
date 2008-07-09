
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef MAP_AVATAR_HPP_INCLUDED
#define MAP_AVATAR_HPP_INCLUDED

#include <GL/glew.h>
#include <GL/gl.h>

#include "location_tracker.hpp"
#include "model_fwd.hpp"
#include "surface.hpp"
#include "wml_node.hpp"

namespace hex
{

class drawable 
{
public:
	typedef GLfloat* float4f; 
    typedef GLfloat* float3f;
    virtual ~drawable() {}

    const virtual float3f position(int key) const =0;
    const virtual float3f scale(int key) const =0;
    const virtual float4f rotation(int key) const =0;
};

class basic_drawable: public drawable {
public:
    basic_drawable() {
        position_[0] = 0.0;
        position_[1] = 0.0;
        position_[2] = 0.0;
        scale_[0] = 1.0;
        scale_[1] = 1.0;
        scale_[2] = 1.0;
        rotation_[0] = 0.0;
        rotation_[1] = 0.0;
        rotation_[2] = 0.0;
        rotation_[3] = 1.0;
    }
    virtual ~basic_drawable() {}

    const float3f position(int key) const { update_position(key); return position_; }
    const float3f scale(int key) const { update_scale(key); return scale_; }
    const float4f rotation(int key) const { update_rotation(key); return rotation_; }
protected:
    GLfloat& position_s(int i) const { return position_[i]; }
    GLfloat& scale_s(int i) const { return scale_[i]; }
    GLfloat& rotation_s(int i) const { return rotation_[i]; }
    GLfloat *position_v() const { return position_; }
    GLfloat *scale_v() const { return scale_; }
    GLfloat *rotation_v() const { return rotation_; }

    virtual void update_position(int key) const {}
    virtual void update_rotation(int key) const {}
    virtual void update_scale(int key) const {}
private:
    mutable GLfloat position_[3];
    mutable GLfloat scale_[3];
    mutable GLfloat rotation_[4];
};

class map_avatar;

typedef boost::shared_ptr<map_avatar> map_avatar_ptr;
typedef boost::shared_ptr<const map_avatar> const_map_avatar_ptr;

class map_avatar
{
public:

    static map_avatar_ptr create(wml::const_node_ptr node,
                                 const drawable* owner,
                                 int key=0);
    static map_avatar_ptr create(graphics::const_model_ptr model,
                                 graphics::surface image,
                                 const drawable* owner,
                                 int key=0);
    virtual ~map_avatar() {}
    bool valid() const;
    void write(wml::node_ptr node) const;
    void draw(bool forSelection=false) const;
    graphics::location_tracker& loc_tracker() const { return loc_tracker_; }
protected:
    map_avatar(wml::const_node_ptr node,
               const drawable* owner,
               int key);
    map_avatar(graphics::const_model_ptr model,
               graphics::surface image,
               const drawable* owner,
               int key);
private:
    void init();

    const drawable* owner_;
    int key_;
    graphics::const_model_ptr model_;
    graphics::surface image_;
    std::string model_id_;
    std::string image_id_;
    bool has_node_;
    mutable graphics::location_tracker loc_tracker_;
};

}

#endif
