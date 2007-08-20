
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
#include "../formatter.hpp"
#include "../gamemap.hpp"
#include "../raster.hpp"
#include "../terrain_feature.hpp"
#include "../tile.hpp"
#include "../wml_parser.hpp"
#include "../wml_utils.hpp"

#include "editorglwidget.hpp"
#include "editpartydialog.hpp"

namespace {
void clone_hex(hex::gamemap& map, const hex::tile& src, const hex::location& loc)
{
	const hex::tile& dst = map.get_tile(loc);
	map.adjust_height(loc,src.height() - dst.height());
	map.set_terrain(loc,src.terrain()->id());
	hex::const_terrain_feature_ptr f = src.feature();
	std::string feature_id;
	if(f) {
		feature_id = f->id();
	}

	map.set_feature(loc,feature_id);
}
}

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

	parties_ = NULL;
	pick_mode_ = false;
	parties_mode_ = false;
	new_mutation_ = true;

	connect(&timer_, SIGNAL(timeout()), this, SLOT(updateIfNeeded()));
	timer_.start(1000/25);
}

void EditorGLWidget::setMap(hex::gamemap *map) {
	map_ = map;
}

void EditorGLWidget::setCamera(hex::camera *camera) {
	camera_ = camera;
	std::cerr << "setting camera_ to " << std::hex << (int)camera << std::endl;
	camera_->set_dim(width(),height());
	std::cerr << "setting dim to " << std::dec << width() << ", " << height() << std::endl;
}

void EditorGLWidget::setParties(party_map* parties) {
	parties_ = parties;
	avatar_.clear();
	if(!parties_) {
		return;
	}

	for(party_map::const_iterator i = parties_->begin();
	    i != parties_->end(); ++i) {
		GLfloat pos[3];
		getAvatarPos(i->first, pos);
		
		avatar_[i->first] = hex::map_avatar::create(i->second,pos);
	}
}

void EditorGLWidget::undo() {
	if(undo_stack_.empty()) {
		return;
	}

	new_mutation_ = true;

	const undo_info& u = undo_stack_.top();
	foreach(const hex::tile& t, u.tiles) {
		clone_hex(*map_, t, t.loc());
	}

	undo_stack_.pop();
	updateGL();
}

void EditorGLWidget::redo() {
}

void EditorGLWidget::setHeightEdit() {
	current_terrain_.clear();
	current_feature_.clear();
	pick_mode_ = false;
	parties_mode_ = false;
	current_party_ = hex::location();
}

void EditorGLWidget::setPicker() {
	current_terrain_.clear();
	current_feature_.clear();
	pick_mode_ = true;
	parties_mode_ = false;
	current_party_ = hex::location();
	picked_loc_ = hex::location();
}

void EditorGLWidget::setCurrentTerrain(const std::string& terrain) {
	current_terrain_ = terrain;
	current_feature_.clear();
	pick_mode_ = false;
	parties_mode_ = false;
	current_party_ = hex::location();
}

void EditorGLWidget::setCurrentFeature(const std::string& terrain) {
	current_terrain_.clear();
	current_feature_ = terrain;
	pick_mode_ = false;
	parties_mode_ = false;
	current_party_ = hex::location();
}

void EditorGLWidget::setEditParties() {
	current_terrain_.clear();
	current_feature_.clear();
	pick_mode_ = false;
	parties_mode_ = true;
	current_party_ = hex::location();
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

void EditorGLWidget::resizeGL(int w, int h) {
	if(camera_) {
		camera_->set_dim(w,h);
	}
}

void EditorGLWidget::paintGL()
{
	if(map_ && camera_) {
		glEnable(GL_LIGHT0);
		std::vector<hex::location> locs;

		GLfloat xscroll = -camera_->get_pan_x();
		GLfloat yscroll = -camera_->get_pan_y();
		const hex::tile* center = map_->closest_tile(&xscroll,&yscroll);
		if(center) {
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

			std::vector<GLuint> selection;

			camera_->finish_selection(&selection);

			selected_ = hex::location();
			int high = -1;
			foreach(GLuint n, selection) {
				if(!map_->is_loc_on_map(locs[n])) {
					continue;
				}

				const hex::tile& t = map_->get_tile(locs[n]);
				
				if(!selected_.valid() || t.height() > high) {
					selected_ = locs[n];
					high = t.height();
				}
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

		if(map_->is_loc_on_map(selected_)) {
			for(int n = 0; n <= radius_; ++n) {
				std::vector<hex::location> locs;
				hex::get_tile_ring(selected_, n, locs);
				foreach(const hex::location& loc, locs) {
					if(map_->is_loc_on_map(loc)) {
						const hex::tile& t = map_->get_tile(loc);
						t.draw_highlight();
					}
				}
			}
		}

		if(map_->is_loc_on_map(current_party_)) {
			map_->get_tile(current_party_).draw_highlight();
		}

		for(avatar_map::const_iterator i = avatar_.begin();
		    i != avatar_.end(); ++i) {
			i->second->draw();
		}

		if(map_->is_loc_on_map(current_party_)) {
			std::cerr << "current party...\n";
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_TEXTURE_2D);
			glBegin(GL_LINES);
			glColor4f(1.0,1.0,1.0,1.0);
			wml::node_vector v = wml::child_nodes((*parties_)[current_party_], "wander");
			std::cerr << "wander: " << v.size() << "\n";
			foreach(const wml::node_ptr& ptr, v) {
				hex::location loc(wml::get_int(ptr,"x"),wml::get_int(ptr,"y"));
				if(!map_->is_loc_on_map(loc)) {
					continue;
				}

				std::cerr << "drawing " << current_party_.x() << "," << current_party_.y() << " -> " << loc.x() << "," << loc.y() << "\n";

				glVertex3f(hex::tile::translate_x(current_party_),
				           hex::tile::translate_y(current_party_),
						   hex::tile::translate_height(
								 map_->get_tile(current_party_).height()));
				glVertex3f(hex::tile::translate_x(loc),
				           hex::tile::translate_y(loc),
						   hex::tile::translate_height(
								 map_->get_tile(loc).height()));
			}
			glEnd();
			glEnable(GL_TEXTURE_2D);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_LIGHTING);
		}

		{
		std::ostringstream s;
		s << "(" << selected_.x() << "," << selected_.y();
		if(map_->is_loc_on_map(selected_)) {
			const hex::tile& t = map_->get_tile(selected_);
			s << "," << t.height();
		}
		s << ")";
		renderText(20,20,s.str().c_str());
		if(pick_mode_) {
			std::string pick_text;
			if(map_->is_loc_on_map(picked_loc_)) {
				const hex::tile& t = map_->get_tile(picked_loc_);
				std::ostringstream s;
				s << "pick: " << t.terrain()->name() << " " << t.height();
				pick_text = s.str();
			} else {
				pick_text = "pick mode";
			}

			renderText(20,50,pick_text.c_str());
		} else if(current_feature_.empty() == false) {
			hex::const_terrain_feature_ptr t = hex::terrain_feature::get(current_feature_);
			if(t) {
				renderText(20,50,t->name().c_str());
			}
		} else {
			hex::const_base_terrain_ptr t = hex::base_terrain::get(current_terrain_);
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

void EditorGLWidget::updateIfNeeded() {
	bool update = camera_->is_moving();
	if (update) {
		updateGL();
	}
}

namespace {
// Functions to determine if a mouse event holds a left click or a right click.
// Meta+click counts as a right-click to work nicely with OSX. Qt doesn't seem to get
// this right for mouse move events.
bool left_button(const QMouseEvent* event)
{
	return (event->buttons()&Qt::LeftButton) && !(event->modifiers()&Qt::MetaModifier);
}

bool right_button(const QMouseEvent* event)
{
	return (event->buttons()&Qt::RightButton) ||
	       ((event->buttons()&Qt::LeftButton) && (event->modifiers()&Qt::MetaModifier));
}
}

void EditorGLWidget::mousePressEvent(QMouseEvent *event)
{
	std::cerr << "buttons: " << (int)event->button() << "," << (int)event->buttons() << ", " << (int)event->modifiers() << "\n";
	std::cerr << "ctrl: " << (int)(event->modifiers()&Qt::ControlModifier) << "\n";
	if(parties_mode_) {
		if(!parties_) {
			return;
		}

		if(left_button(event)) {
			if(!map_->is_loc_on_map(selected_)) {
				return;
			}

			if(current_party_.valid() && parties_->count(current_party_)) {
				wml::node_ptr party = (*parties_)[current_party_];
				wml::node_vector v = wml::child_nodes(party, "wander");
				bool found = false;
				foreach(const wml::node_ptr& p, v) {
					if(hex::location(wml::get_int(p,"x"),wml::get_int(p,"y")) == selected_) {
						found = true;
						party->erase_child(p);
						break;
					}
				}

				if(!found) {
					wml::node_ptr wander(new wml::node("wander"));
					wander->set_attr("x",formatter() << selected_.x());
					wander->set_attr("y",formatter() << selected_.y());
					party->add_child(wander);
					std::cerr << "add wander...\n";
				}
			} else {
				wml::node_ptr& p = (*parties_)[selected_];
				if(!p) {
					p.reset(new wml::node("party"));
					p->set_attr("x",formatter() << selected_.x());
					p->set_attr("y",formatter() << selected_.y());
				}
				EditPartyDialog d(p);
				d.exec();
				if(!p->get_child("character")) {
					parties_->erase(selected_);
				}
				setParties(parties_);
			}
		} else if(right_button(event)) {
			if(map_->is_loc_on_map(selected_) && parties_->count(selected_)) {
				current_party_ = selected_;
			} else {
				current_party_ = hex::location();
			}
		}
	} else {
		modifyHex(event);
	}
	updateGL();
}

void EditorGLWidget::mouseMoveEvent(QMouseEvent *event)
{
	std::cerr << "move: " << (int)event->button() << "," << (int)event->buttons() << "\n";
	mousex_ = event->x();
	mousey_ = event->y();
	setFocus(Qt::MouseFocusReason);
	if(left_button(event) || right_button(event)) {
		if(!parties_mode_) {
			modifyHex(event);
		}
	}
	updateGL();
}

void EditorGLWidget::modifyHex(QMouseEvent* event)
{
	int adjust = 0;
	if(left_button(event)) {
		adjust = 1;
	} else if(right_button(event)) {
		adjust = -1;
		if(pick_mode_) {
			picked_loc_ = selected_;
		}
	}
	if(adjust != 0 && map_->is_loc_on_map(selected_)) {
		if(new_mutation_) {
			new_mutation_ = false;
			undo_stack_.push(undo_info());
		}
		assert(!undo_stack_.empty());
		undo_info& undo = undo_stack_.top();

		for(int n = 0; n <= radius_; ++n) {
			std::vector<hex::location> locs;
			hex::get_tile_ring(selected_, n, locs);
			foreach(const hex::location& loc, locs) {
				if(map_->is_loc_on_map(loc)) {
					if(undo.locs.count(loc) == 0) {
						undo.tiles.push_back(map_->get_tile(loc));
						undo.locs.insert(loc);
					}

					if(pick_mode_) {
						if(adjust == 1 && map_->is_loc_on_map(picked_loc_)) {
							const hex::tile& src = map_->get_tile(picked_loc_);
							const hex::tile& dst = map_->get_tile(loc);
							map_->adjust_height(loc,src.height() - dst.height());
							map_->set_terrain(loc,src.terrain()->id());
							hex::const_terrain_feature_ptr f = src.feature();
							std::string feature_id;
							if(f) {
								feature_id = f->id();
							}

							map_->set_feature(loc,feature_id);
						}
					} else if(!current_feature_.empty()) {
						map_->set_feature(loc,current_feature_);
					} else if(current_terrain_.empty()) {
						map_->adjust_height(loc,adjust);
					} else {
						map_->set_terrain(loc,current_terrain_);
					}
				}

				avatar_map::iterator av = avatar_.find(loc);
				if(av != avatar_.end()) {
					getAvatarPos(loc,av->second->position_buffer());
				}
			}
		}
	} else {
		new_mutation_ = true;
	}
}

void EditorGLWidget::getAvatarPos(const hex::location& loc, GLfloat* pos) {
	if(!map_->is_loc_on_map(loc)) {
		return;
	}
	pos[0] = hex::tile::translate_x(loc);
	pos[1] = hex::tile::translate_y(loc);
	pos[2] = hex::tile::translate_height(map_->get_tile(loc).height());
}

#include "editorglwidget.moc"
