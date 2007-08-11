
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <gl.h>
#include <glu.h>

#include <cmath>
#include <iostream>
#include <set>
#include <sstream>
#include <stack>
#include <vector>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <QtGui/QKeyEvent>

#include "../base_terrain.hpp"
#include "../camera.hpp"
#include "../filesystem.hpp"
#include "../font.hpp"
#include "../foreach.hpp"
#include "../gamemap.hpp"
#include "../raster.hpp"
#include "../terrain_feature.hpp"
#include "../tile.hpp"
#include "../wml_parser.hpp"

#include "editorglwidget.hpp"

EditorGLWidget::EditorGLWidget(QWidget *parent)
	: QGLWidget(parent)
{
	setMouseTracking(true);
	show_grid_ = true;
	radius_ = 0;
	mousex_ = 0;
	mousey_ = 0;
	map_ = 0;
	camera_ = 0;
	connect(&timer_, SIGNAL(timeout()), this, SLOT(checkKeys()));
	timer_.start(1000/25);
}

void EditorGLWidget::setMap(hex::gamemap *map) {
	map_ = map;
}

void EditorGLWidget::setCamera(hex::camera *camera) {
	camera_ = camera;
}

void EditorGLWidget::initializeGL() 
{
	GLfloat material_specular[] = {1.0,1.0,1.0,1.0};
	GLfloat material_shininess[] = {1000.0};
	GLfloat intensity = 1.0;
	GLfloat ambient_light[] = {intensity,intensity,intensity,1.0};
	GLfloat diffuse_light[] = {1.0,1.0,1.0,1.0};

	GLfloat dim_ambient[] = {0.5,0.5,0.5,1.0};
	GLfloat dim_diffuse[] = {0.5,0.5,0.5,1.0};

	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,material_specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,material_specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,material_specular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,material_shininess);

	glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0,GL_AMBIENT,ambient_light);
	glLightfv(GL_LIGHT0,GL_DIFFUSE,diffuse_light);
	glLightfv(GL_LIGHT2,GL_AMBIENT,dim_ambient);
	glLightfv(GL_LIGHT2,GL_DIFFUSE,dim_diffuse);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER,0.05);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
	glPixelStorei(GL_UNPACK_ALIGNMENT,1);
	
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
}

void EditorGLWidget::paintGL()
{
	if(map_ && camera_) {
		glEnable(GL_LIGHT0);
		hex::location selected;
		std::vector<hex::location> locs;

		bool pick_mode = false;
		hex::location picked_loc;
		std::string current_terrain;
		std::string current_feature;
		bool new_mutation = true;

		GLfloat xscroll = -camera_->get_pan_x();
		GLfloat yscroll = -camera_->get_pan_y();
		const hex::tile* center = map_->closest_tile(&xscroll,&yscroll);
		if(center) {
			std::cout << "Selecting at " << mousex_ << "," << mousey_ << std::endl;
			camera_->prepare_selection(mousex_,mousey_);
			locs.clear();
			GLuint select_name = 0;
			for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
				for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
					hex::location loc(x,y);
					if(map_->is_loc_on_map(loc)) {
						const hex::tile& t = map_->get_tile(loc);
						locs.push_back(loc);
						glLoadName(select_name++);
						t.draw();
					}
				}
			}

			select_name = camera_->finish_selection();
			if(select_name < locs.size()) {
				selected = locs[select_name];
			}

			camera_->prepare_frame();

			for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
				for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
					hex::location loc(x,y);
					if(map_->is_loc_on_map(loc)) {
						const hex::tile& t = map_->get_tile(loc);
						t.draw();
						t.draw_model();
					}
				}
			}

			for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
				for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
					hex::location loc(x,y);
					if(map_->is_loc_on_map(loc)) {
						const hex::tile& t = map_->get_tile(loc);
						t.draw_cliffs();
					}
				}
			}

			if(show_grid_) {
				glDisable(GL_LIGHTING);
				for(int x = center->loc().x()-20; x < center->loc().x()+20; ++x) {
					for(int y = center->loc().y()-20; y < center->loc().y()+20; ++y) {
						hex::location loc(x,y);
						if(map_->is_loc_on_map(loc)) {
							const hex::tile& t = map_->get_tile(loc);
							t.draw_grid();
						}
					}
				}
				glEnable(GL_LIGHTING);
			}
		} else {
			map_->draw();
			map_->draw_grid();
		}

		if(map_->is_loc_on_map(selected)) {
			for(int n = 0; n <= radius_; ++n) {
				std::vector<hex::location> locs;
				hex::get_tile_ring(selected, n, locs);
				foreach(const hex::location& loc, locs) {
					if(map_->is_loc_on_map(loc)) {
						const hex::tile& t = map_->get_tile(loc);
						t.draw_highlight();
					}
				}
			}
		}

		{
		std::ostringstream s;
		s << "(" << selected.x() << "," << selected.y();
		if(map_->is_loc_on_map(selected)) {
			const hex::tile& t = map_->get_tile(selected);
			s << "," << t.height();
		}
		s << ")";
		renderText(20,20,s.str().c_str());
		if(pick_mode) {
			std::string pick_text;
			if(map_->is_loc_on_map(picked_loc)) {
				const hex::tile& t = map_->get_tile(picked_loc);
				std::ostringstream s;
				s << "pick: " << t.terrain()->name() << " " << t.height();
				pick_text = s.str();
			} else {
				pick_text = "pick mode";
			}

			renderText(20,50,pick_text.c_str());
		} else if(current_feature.empty() == false) {
			hex::const_terrain_feature_ptr t = hex::terrain_feature::get(current_feature);
			if(t) {
				renderText(20,50,t->name().c_str());
			}
		} else {
			hex::const_base_terrain_ptr t = hex::base_terrain::get(current_terrain);
			if(t) {
				renderText(20,50,t->name().c_str());
			}
		}
		}
	}
}

void EditorGLWidget::keyPressEvent(QKeyEvent *event)
{
	keys_.insert(event->key(),true);

	int key = event->key();
	if(key == Qt::Key_G) {
		show_grid_ = true;
		std::cerr << "Enabling grid" << std::endl;
	} else if(key == Qt::Key_H) {
		show_grid_ = false;
		std::cerr << "Disabling grid" << std::endl;
	} else if(key >= Qt::Key_0 && key <= Qt::Key_9) {
		radius_ = key - Qt::Key_0;
		std::cerr << "Changing radius" << std::endl;
	} else {
		event->ignore();
	}
	updateGL();
}

void EditorGLWidget::enterEvent(QEvent *event) {
	keys_.clear();
}

void EditorGLWidget::leaveEvent(QEvent *event) {
	keys_.clear();
}

void EditorGLWidget::keyReleaseEvent(QKeyEvent *event) {
	keys_.remove(event->key());
}

void EditorGLWidget::checkKeys() {
	bool update = false;
	if (keys_.contains(Qt::Key_Up)) {
		update = true;
		camera_->pan_up();
	} else if(keys_.contains(Qt::Key_Down)) {
		update = true;
		camera_->pan_down();
	} else if(keys_.contains(Qt::Key_Left)) {
		update = true;
		camera_->pan_left();
	} else if(keys_.contains(Qt::Key_Right)) {
		update = true;
		camera_->pan_right();
	}
	if (update) {
		updateGL();
	}
}

void EditorGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	mousex_ = event->x();
	mousey_ = event->y();
	setFocus(Qt::MouseFocusReason);
	updateGL();
}

#include "editorglwidget.moc"
