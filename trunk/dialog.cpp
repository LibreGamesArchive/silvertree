
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
#include "SDL.h"

#include <iostream>

#include "dialog.hpp"
#include "font.hpp"
#include "foreach.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"
#include "texture.hpp"
#include "tooltip.hpp"

namespace gui {

dialog::dialog(int x, int y, int w, int h)
  : opened_(false), padding_(10), add_x_(0), add_y_(0)
{
	set_loc(x,y);
	set_dim(w,h);
}

dialog& dialog::add_widget(widget_ptr w, dialog::MOVE_DIRECTION dir)
{
	add_widget(w, add_x_, add_y_, dir);
	return *this;
}

dialog& dialog::add_widget(widget_ptr w, int x, int y,
                           dialog::MOVE_DIRECTION dir)
{
	w->set_loc(x,y);
	widgets_.push_back(w);
	switch(dir) {
	case MOVE_DOWN:
		add_x_ = x;
		add_y_ = y + w->height() + padding_;
		break;
	case MOVE_RIGHT:
		add_x_ = x + w->width() + padding_;
		add_y_ = y;
		break;
	}
	return *this;
}

void dialog::remove_widget(widget_ptr w)
{
	widgets_.erase(std::remove(widgets_.begin(),widgets_.end(),w),
	               widgets_.end());
}

void dialog::show_modal()
{
	opened_ = true;
	while(opened_) {
		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			process_event(event);
		}

		prepare_draw();
		draw();
		gui::draw_tooltip();
		complete_draw();
	}
}

void dialog::prepare_draw()
{
	graphics::prepare_raster();
	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void dialog::complete_draw()
{
	SDL_GL_SwapBuffers();
	SDL_Delay(1);
}

void dialog::handle_draw() const
{
	SDL_Rect rect = {x(),y(),width(),height()};
	SDL_Color col = {0,0,0,0};
	graphics::draw_rect(rect,col);
	glPushMatrix();
	glTranslatef(x(),y(),0.0);
	foreach(const widget_ptr& w, widgets_) {
		w->draw();
	}
	glPopMatrix();
}

void dialog::handle_event(const SDL_Event& event)
{
	if(event.type == SDL_KEYDOWN &&
	   (event.key.keysym.sym == SDLK_SPACE ||
	    event.key.keysym.sym == SDLK_RETURN)) {
		close();
	}

	SDL_Event ev = event;
	normalize_event(&ev);
	std::vector<widget_ptr> widgets = widgets_;
	foreach(const widget_ptr& w, widgets) {
		w->process_event(ev);
	}
}

int show_dialog(const std::string& msg,
                const std::vector<std::string>* options)
{
	std::cerr << "show dialog: '" << msg << "'\n";
	if(options == NULL) {
		std::cerr << "no options\n";
	} else {
		for(std::vector<std::string>::const_iterator o = options->begin(); o != options->end(); ++o) {
			std::cerr << "option: '" << *o << "'\n";
		}
	}
	SDL_Color color = {0xFF,0xFF,0xFF,0xFF};
	std::vector<graphics::texture> text;
	int width, height;
	graphics::font::render_multiline_text(msg, 18, color, text, &width, &height);
	std::vector<graphics::texture> options_text;
	if(options) {
		foreach(const std::string& s, *options) {
			options_text.push_back(graphics::font::render_text(s, 18, color));
			height += options_text.back().height();
			if(options_text.back().width() > width) {
				width = options_text.back().width();
			}
		}
	}

	const int border = 10;

	int option = 0;
	bool done = false;
	bool redraw = true;
	while(!done) {

		if(redraw) {
			int x = 1024/2 - width/2;
			int y = 768/2 - height/2;
			graphics::prepare_raster();
			graphics::texture bg(graphics::texture::get(graphics::surface_cache::get("dialog-background.png")));
			bg.set_as_current_texture();
			glBegin(GL_QUADS);
			glColor3f(1.0,1.0,1.0);
			graphics::texture::set_coord(0.0,0.0);
			glVertex3i(x-border,y-border,0);
			graphics::texture::set_coord(0.0,1.0);
			glVertex3i(x-border,y+height+border,0);
			graphics::texture::set_coord(1.0,1.0);
			glVertex3i(x+width+border,y+height+border,0);
			graphics::texture::set_coord(1.0,0.0);
			glVertex3i(x+width+border,y-border,0);
			glEnd();
			foreach(const graphics::texture& t, text) {
				graphics::blit_texture(t, x, y);
				y += t.height();
			}

			int option_num = 0;
			foreach(const graphics::texture& t, options_text) {
				if(option_num == option) {
					glDisable(GL_TEXTURE_2D);
					glColor3f(1.0,0.0,0.0);
					glRectf(x,y,x+t.width(),y+t.height());
					glEnable(GL_TEXTURE_2D);
				}

				graphics::blit_texture(t, x, y);
				y += t.height();
				++option_num;
			}
			SDL_GL_SwapBuffers();
			redraw = false;
		}

		SDL_Event event;
		while(SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_KEYDOWN:
				if(options) {
					switch(event.key.keysym.sym) {
					case SDLK_UP:
						if(option > 0) {
							--option;
							redraw = true;
						}
						break;
					case SDLK_DOWN:
						if(option < options->size()-1) {
							++option;
							redraw = true;
						}
						break;
					case SDLK_RETURN:
					case SDLK_SPACE:
						done = true;
						break;
					}
				} else {
					done = true;
				}
				break;
			default:
				break;
			}
		}
	}

	return option;
}
		
}
