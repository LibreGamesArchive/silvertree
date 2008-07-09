
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "camera.hpp"
#include "gamemap.hpp"
#include "raster.hpp"

#include <GL/glew.h>

#include <cmath>
#include <iostream>

namespace hex
{

namespace {

GLfloat PanSpeed = 0.1;
GLfloat RotateSpeed = 3.0;
GLfloat TiltSpeed = 1.0;
GLfloat ZoomSpeed = 1.0;
GLfloat MinZoom = -100.0;
GLfloat MaxZoom = -15.0;
GLfloat MinTilt = -60.0;
GLfloat MaxTilt = 0.0;

const GLfloat PI = 3.14159265;

camera* cur_camera = NULL;

}

camera* camera::current_camera()
{
    return cur_camera;
}

camera::camera(const gamemap& m)
   : map_(m), width_(graphics::screen_width()), height_(graphics::screen_height()),
     translatex_(0.0), translatey_(0.0),
     translatez_(0.0),
     rotate_(0.0), dir_(NORTH),
     tilt_(-30.0), zoom_(-30.0), debug_adjust_(false),
     keyboard_pan_(false),
     moved_since_last_check_(true)
{
    std::fill(background_,background_+sizeof(background_)/sizeof(*background_),0.0);
}

camera::~camera()
{
    if(cur_camera == this) {
        cur_camera = NULL;
    }
}

void camera::prepare_frame()
{
    cur_camera = this;
    enforce_limits();
    
    //glEnable(GL_LINE_SMOOTH);
    
    glShadeModel(GL_SMOOTH);
    glClearColor(background_[0],background_[1],background_[2],0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glFogfv(GL_FOG_COLOR, background_);
    
    glViewport(0,0,width_,height_);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 160.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0,0.0,zoom_);
    glRotatef(tilt_,1.0,0.0,0.0);
    glRotatef(rotate_,0.0,0.0,1.0);
    glTranslatef(translatex_,translatey_,translatez_);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    
    glFogf(GL_FOG_START, 30.0f - zoom_);
    glFogf(GL_FOG_END, 60.0f - zoom_);
}

void camera::prepare_selection(int mousex, int mousey)
{
    selection_.resize(1000);
    glSelectBuffer(selection_.size(), &selection_[0]);
    
    glRenderMode(GL_SELECT);
    
    glInitNames();
    glPushName(0);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    const double x = double(mousex);
    const double y = GLfloat(height_) - double(mousey);
    const double radius = 1.0;
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gluPickMatrix(x, y, radius, radius, viewport);
    glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 160.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0,0.0,zoom_);
    glRotatef(tilt_,1.0,0.0,0.0);
    glRotatef(rotate_,0.0,0.0,1.0);
    glTranslatef(translatex_,translatey_,translatez_);
    
    glDisable(GL_LIGHTING);
}

GLuint camera::finish_selection(std::vector<GLuint>* items)
{
    glEnable(GL_LIGHTING);
    glPopName();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glFlush();
    GLint hits = glRenderMode(GL_RENDER);
    selection_.resize(hits);
    std::vector<GLuint>::const_iterator itor = selection_.begin();
    GLuint res = GLuint(-1);
    GLuint closest = GLuint(-1);
    for(GLint i = 0; i < hits && itor != selection_.end(); ++i) {
        GLuint names = *itor++;
        const GLuint z1 = *itor++;
        ++itor;
        if(names > 0 && (closest == GLuint(-1) || z1 > closest)) {
            res = *itor;
            closest = z1;
        }
        
        if(items) {
            items->push_back(res);
        }
        
        itor += names;
    }
    
    return res;
}

void camera::pan_up()
{
    translatex_ -= PanSpeed*std::sin(rotate_radians());
    translatey_ -= PanSpeed*std::cos(rotate_radians());
}

void camera::pan_down()
{
    translatex_ += PanSpeed*std::sin(rotate_radians());
    translatey_ += PanSpeed*std::cos(rotate_radians());
}

void camera::pan_left()
{
    translatex_ += PanSpeed*std::cos(rotate_radians());
    translatey_ -= PanSpeed*std::sin(rotate_radians());
}

void camera::pan_right()
{
    translatex_ -= PanSpeed*std::cos(rotate_radians());
    translatey_ += PanSpeed*std::sin(rotate_radians());
}

void camera::set_pan(const GLfloat* buf)
{
    if(translatex_ != -buf[0] || translatey_ != -buf[1] || translatez_ != -buf[2]) {
        moved_since_last_check_ = true;
        translatex_ = -buf[0];
        translatey_ = -buf[1];
        translatez_ = -buf[2];
    }
}

GLfloat camera::rotate_radians() const
{
    return (rotate_/180.0)*PI;
}

void camera::rotate_left()
{
    moved_since_last_check_ = true;
    if(need_to_rotate() > 0 || need_to_rotate() > -RotateSpeed) {
        int dir = static_cast<int>(dir_);
        --dir;
        if(dir < 0) {
            dir = 5;
        }
        
        dir_ = static_cast<DIRECTION>(dir);
    }
}

void camera::rotate_right()
{
    moved_since_last_check_ = true;
    if(need_to_rotate() < 0 || need_to_rotate() < RotateSpeed) {
        int dir = static_cast<int>(dir_);
        ++dir;
        if(dir > 5) {
            dir = 0;
        }
        
        dir_ = static_cast<DIRECTION>(dir);
    }
}

void camera::tilt_up(GLfloat speed)
{
    tilt_ += speed;
    moved_since_last_check_ = true;
}

void camera::tilt_up()
{
    camera::tilt_up(TiltSpeed);
}

void camera::tilt_down(GLfloat speed)
{
    tilt_ -= speed;
    moved_since_last_check_ = true;
}

void camera::tilt_down()
{
    camera::tilt_down(TiltSpeed);
}

void camera::zoom_in()
{
    zoom_ += ZoomSpeed;
    moved_since_last_check_ = true;
}

void camera::zoom_out()
{
    zoom_ -= ZoomSpeed;
    moved_since_last_check_ = true;
}

GLfloat camera::max_zoom() const
{
    return MaxZoom;
}

GLfloat camera::min_zoom() const
{
    return MinZoom;
}

DIRECTION camera::direction() const
{
    switch(dir_) {
    case NORTH:
        return SOUTH;
    case NORTH_EAST:
        return SOUTH_EAST;
    case SOUTH_EAST:
        return NORTH_EAST;
    case SOUTH:
        return NORTH;
    case SOUTH_WEST:
        return NORTH_WEST;
    case NORTH_WEST:
        return SOUTH_WEST;
    }
    
    assert(false);
}

void camera::enforce_limits()
{
    if(zoom_ > MaxZoom) {
        zoom_ = MaxZoom;
    }
    
    if(zoom_ < MinZoom) {
        zoom_ = MinZoom;
    }
    
    if(tilt_ > MaxTilt) {
	//	tilt_ = MaxTilt;
    }
    
    if(tilt_ < MinTilt) {
	//	tilt_ = MinTilt;
    }
    
    if(translatex_ > 0.0) {
        translatex_ = 0.0;
    }
    
    if(translatey_ > 0.0) {
        translatey_ = 0.0;
    }
    
    if(translatex_ < -static_cast<GLfloat>(map_.size().x())) {
        translatex_ = -static_cast<GLfloat>(map_.size().x());
    }
    
    if(translatey_ < -static_cast<GLfloat>(map_.size().y())) {
        translatey_ = -static_cast<GLfloat>(map_.size().y());
    }
    
    const GLfloat rotate = need_to_rotate();
    if(std::abs(rotate) < RotateSpeed) {
        rotate_ = target_rotation();
    } else if(rotate > 0.0) {
        moved_since_last_check_ = true;
        rotate_ += RotateSpeed;
    } else if(rotate < 0.0) {
        moved_since_last_check_ = true;
        rotate_ -= RotateSpeed;
    }
    
    while(rotate_ > 360.0) {
        rotate_ -= 360.0;
    }
    
    while(rotate_ < 0.0) {
        rotate_ += 360.0;
    }
}

GLfloat camera::target_rotation() const
{
    return static_cast<GLfloat>(dir_)*60.0;
}

GLfloat camera::need_to_rotate() const
{
    const GLfloat target = target_rotation();
    const GLfloat normal = target - rotate_;
    const GLfloat reverse = rotate_ > target ?
        360.0 - rotate_ + target : -(rotate_ + 360.0 - target);
    
    if(std::abs(normal) < std::abs(reverse)) {
        return normal;
    } else {
        return reverse;
    }
}

bool camera::is_moving() const
{
    return target_rotation() != rotate_;
}

void camera::set_background_color(const GLfloat* col) {
    for(int n = 0; n != 3; ++n) {
        background_[n] = col[n];
    }
}

} //end namespace hex
