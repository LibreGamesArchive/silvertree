
/*
   Copyright (C) 2007 by David White <dave.net>
   Copyright (C) 2007 by Isaac Clerencia <isaac@warp.es>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef EDITORGLWIDGET_HPP_INCLUDED
#define EDITORGLWIDGET_HPP_INCLUDED

#include <QtCore/QTimer>
#include <QtOpenGL/QGLWidget>

#include <set>
#include <stack>

#include "../camera.hpp"
#include "../gamemap.hpp"

struct undo_info {
	std::vector<hex::tile> tiles;
	std::set<hex::location> locs;
};

class EditorGLWidget : public QGLWidget
{
	Q_OBJECT

	public:
		EditorGLWidget(QWidget *parent = 0);
		void setMap(hex::gamemap *map);
		void setCamera(hex::camera *camera);
		void undo();
		void redo();
		void setHeightEdit();
		void setPicker();
		void setCurrentTerrain(const std::string& terrain);
		void setCurrentFeature(const std::string& terrain);

	protected:
		void initializeGL();
		void resizeGL(int w, int h);
		void paintGL();

		void enterEvent(QEvent *event);
		void leaveEvent(QEvent *event);
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
		void mousePressEvent(QMouseEvent *event);
		void mouseMoveEvent(QMouseEvent *event);

		void modifyHex(QMouseEvent* event);

	protected slots:
		void checkKeys();

	private:
		hex::gamemap *map_;
		hex::camera *camera_;
		hex::location selected_;
		bool show_grid_;
		int radius_;
		int mousex_;
		int mousey_;

		bool pick_mode_;
		hex::location picked_loc_;
		std::string current_terrain_;
		std::string current_feature_;
		bool new_mutation_;

		std::stack<undo_info> undo_stack_;
		QTimer timer_;
		QMap<int,bool> keys_;
};

#endif
