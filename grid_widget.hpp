
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef GRID_WIDGET_HPP_INCLUDED
#define GRID_WIDGET_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <vector>

#include "callback.hpp"
#include "grid_widget_fwd.hpp"
#include "widget.hpp"

namespace gui {

class grid : public widget
{
public:
	typedef functional::callback_arg<int>::ptr select_callback_ptr;
	enum COLUMN_ALIGN { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

	explicit grid(int ncols);
	grid& set_show_background(bool val) {
		show_background_ = val;
		return *this;
	}
	void add_row(const std::vector<widget_ptr>& widgets);

	grid& add_col(const widget_ptr& widget) {
		new_row_.push_back(widget);
		if(new_row_.size() == ncols_) {
			add_row(new_row_);
			new_row_.clear();
		}
		return *this;
	}

	grid& set_col_width(int col, int width);
	grid& set_align(int col, COLUMN_ALIGN align);
	grid& set_hpad(int pad) { hpad_ = pad; }

	void allow_selection(bool val=true) { allow_selection_ = val; }
	void must_select(bool val=true) { must_select_ = val; selected_row_ = 0; }
	int selection() const { return selected_row_; }
	void register_mouseover_callback(select_callback_ptr ptr);
	void register_selection_callback(select_callback_ptr ptr);
private:
	int row_at(int x, int y) const;
	void recalculate_dimensions();
	void handle_draw() const;
	void handle_event(const SDL_Event& event);

	int nrows() const { return cells_.size()/ncols_; }
	int ncols_;
	std::vector<widget_ptr> cells_;
	std::vector<int> col_widths_;
	std::vector<COLUMN_ALIGN> col_aligns_;
	int row_height_;
	int selected_row_;
	bool allow_selection_;
	bool must_select_;

	std::vector<widget_ptr> new_row_;
	select_callback_ptr on_mouseover_;
	select_callback_ptr on_select_;
	int hpad_;
	bool show_background_;
};

typedef boost::shared_ptr<grid> grid_ptr;
typedef boost::shared_ptr<const grid> const_grid_ptr;

}

#endif
