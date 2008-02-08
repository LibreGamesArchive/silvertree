#include <algorithm>

#include "gui_core.hpp"
#include "label.hpp"
#include "raster.hpp"

namespace gui {

skinned_widget::skinned_widget()
	: state_(0),
	  have_real_dim_(false), have_real_loc_(false), nested_draw_(false),
	  inner_(new delegate_widget(this)) { }

void skinned_widget::set_state(int state) const
{
	if(state_ == state) {
		return;
	}
	state_ = state;
	if(state < 0) {
		state_ = 0;
	}

	assert(state < frames_.size());

	// ugly magic necessary to allow both switching state in redraw
	// and automatic repositioning on frame change
	skinned_widget *non_const_this = const_cast<skinned_widget *>(this);

	if(have_real_loc_) {
		non_const_this->set_loc(real_dims_.x, real_dims_.y);
	} else {
		non_const_this->inner_set_loc(frames_[state_]->x(), frames_[state_]->y());
	}
	if(have_real_dim_) {
		non_const_this->set_dim(real_dims_.w, real_dims_.h);
	} else {
		non_const_this->inner_set_dim(frames_[state_]->width(), frames_[state_]->height());
	}

}

int skinned_widget::add_skin(const std::string &name, int state)
{
	int ret;

	frame_ptr frame = frame_manager::make_frame(inner_, name);

	if(state >= 0) {
		if(state+1 > frames_.size()) {
			for(int i = frames_.size(); i < state; ++i) {
				cancelled_frames_.insert(cancelled_frames_.begin(), i);
			}
			frames_.resize(state+1);
		}
		frames_[state] = frame;
		ret =  state;
	} else if(cancelled_frames_.empty()) {
		frames_.push_back(frame);
		ret = frames_.size()-1;
	} else {

		std::set<int>::iterator i = cancelled_frames_.begin();
		const int idx = *i;
		frames_[idx] = frame;
		cancelled_frames_.erase(i);
		ret = idx;
	}

	if(state_ == state) {
		if(have_real_loc_) {
			set_loc(real_dims_.x, real_dims_.y);
		} else {
			inner_set_loc(frame->x(), frame->y());
		}
		if(have_real_dim_) {
			set_dim(real_dims_.w, real_dims_.h);
		} else {
			inner_set_dim(frame->width(), frame->height());
		}
	}

	return ret;
}

void skinned_widget::remove_skin(int state) {
	if(state < 0 || state >= frames_.size()) {
		return;
	}
	if(cancelled_frames_.find(state) != cancelled_frames_.end()) {
		return;
	}
	if(state != frames_.size()-1) {
		cancelled_frames_.insert(cancelled_frames_.begin(), state);
		return;
	}
	frames_.erase(frames_.end());

	for(std::set<int>::reverse_iterator i = cancelled_frames_.rbegin();
	    i != cancelled_frames_.rend(); ++i) {
		if(*i != frames_.size()-1) {
			break;
		}
		frames_.erase(frames_.end());
		cancelled_frames_.erase(*i);
	}
}

bool skinned_widget::valid_frame() const
{
	if(state_ < 0 || state_ >= frames_.size()) {
		return false;
	}
	if(cancelled_frames_.find(state_) != cancelled_frames_.end()) {
		return false;
	}
	return true;
}

void skinned_widget::handle_draw() const
{
	if(nested_draw_) {
		inner_draw();
		return;
	}

	prepare_draw();

	if(!valid_frame()) {
		inner_draw();
	} else {
		nested_draw_ = true;
		try {
			frames_[state_]->draw();
		} catch(...) {
			nested_draw_ = false;
			throw;
		}
		nested_draw_ = false;
	}
	finish_draw();
}

void skinned_widget::set_dim(int w, int h)
{
	have_real_dim_ = true;
	real_dims_.w = w;
	real_dims_.h = h;

	if(!valid_frame()) {
		inner_set_dim(w,h);
		return;
	}

	frames_[state_]->set_dim(w,h);
	inner_set_dim(frames_[state_]->width(), frames_[state_]->height());
}

void skinned_widget::inner_set_dim(int w, int h) {
	widget::set_dim(w,h);
}

void skinned_widget::set_loc(int x, int y)
{
	have_real_loc_ = true;
	real_dims_.x = x;
	real_dims_.y = y;

	if(!valid_frame()) {
		inner_set_loc(x,y);
		return;
	}

	frames_[state_]->set_loc(x,y);
	inner_set_loc(frames_[state_]->x(), frames_[state_]->y());
}

void skinned_widget::inner_set_loc(int x, int y)
{
	widget::set_loc(x,y);
}

bool skinned_widget::hit_me(const SDL_Event &e) {
	switch(e.type) {
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		if(e.button.x >  x() && e.button.y > y() &&
		   e.button.x < x() + width() && e.button.y < y() + height())
		{
			return true;
		}
		return false;
	case SDL_MOUSEMOTION:
		if(e.motion.x >  x() && e.motion.y > y() &&
		   e.motion.x < x() + width() && e.motion.y < y() + height())
		{
			return true;
		}
		return false;
	default:
		return false;
	}
}


void button_widget::do_click()
{
	// NB heuristic since finding mouse pos is hard
	const int start = SDL_GetTicks();
	clicked();
	const int end = SDL_GetTicks();
	if(end - start < 20) {
		set_state(HIGHLIGHTED);
	} else {
		set_state(NORMAL);
	}
}

bool button_widget::handle_event(const SDL_Event &e)
{
	if(state() == DISABLED) {
		return false;
	}

	bool claimed = false;

	switch(e.type) {
	case SDL_MOUSEBUTTONDOWN:
		if(hit_me(e)) {
			ready_ = true;
			set_state(CLICKED);
		} else {
			set_state(NORMAL);
		}
		break;
	case SDL_MOUSEBUTTONUP:
	        {
			if(ready_ && hit_me(e)) {
				do_click();
				claimed = true;
			}
			ready_ = false;
		}
		break;
	case SDL_MOUSEMOTION:
		if(hit_me(e)) {
			if(ready_) {
				set_state(CLICKED);
			} else {
				set_state(HIGHLIGHTED);
			}
		} else {
			set_state(NORMAL);
		}
		break;
	case SDL_KEYDOWN:
		if(has_hotkey_) {
			if(e.key.keysym.sym == hotkey_.sym &&
			   e.key.keysym.mod == hotkey_.mod) {
				set_state(CLICKED);
				claimed = true;
			}
		}
		break;
	case SDL_KEYUP:
		if(has_hotkey_) {
			if(e.key.keysym.sym == hotkey_.sym &&
			   e.key.keysym.mod == hotkey_.mod) {
				do_click();
				claimed = true;
			}
		}
		break;
	default:
		break;
	}

	return claimed;
}

bool button_widget::is_enabled() const
{
	return state() != DISABLED;
}
void button_widget::set_enabled(bool enabled)
{
	if(is_enabled() == enabled) {
		return;
	}
	if(enabled) {
		set_state(NORMAL);
	} else {
		set_state(DISABLED);
	}
}


void scrolled_container::add_widget(widget_ptr p)
{
	p->set_loc(0,0);
	widgets_.push_back(p);
}

void scrolled_container::remove_widget(widget_ptr p) {
	widgets_.erase(std::find(widgets_.begin(), widgets_.end(), p));
}

void scrolled_container::handle_draw() const
{
	SDL_Rect self = { x(), y(), width(), height() };
	graphics::push_clip(self);
	glPushMatrix();
	glTranslatef(x(), y(), 0);

	const int max_widget = widgets_.size();
	int widget = offset_;
	int p_used = 0;
	int max_p = axis() == HORIZONTAL ? width() : height();
	while(widget < max_widget && p_used < max_p) {
		int d_used = axis() == HORIZONTAL ? widgets_[widget]->width() : widgets_[widget]->height();
		widgets_[widget]->draw();
		p_used += d_used;
		++widget;
		if(axis() == HORIZONTAL) {
			glTranslatef(d_used, 0, 0);
		} else {
			glTranslatef(0, d_used, 0);
		}
	}

	graphics::pop_clip();
	glPopMatrix();
}

void scrolled_container::scroll(int dir)
{
	set_offset(offset_ + dir);
}

void scrolled_container::set_offset(int pos) {
	offset_ = pos;
	if(offset_ < 0) {
		offset_ = 0;
	}
	if(offset_ >= widgets_.size()) {
		offset_ = widgets_.size() - 1;
	}
}

void scrolled_container::move_event(SDL_Event *ep, int dx, int dy) {
	switch(ep->type) {
	case SDL_MOUSEMOTION:
		ep->motion.x -= dx;
		ep->motion.y -= dy;
		break;
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		ep->button.x -= dx;
		ep->button.y -= dy;
		break;
	default:
		break;
	}
}

bool scrolled_container::handle_event(const SDL_Event &e)
{
	bool claimed = false;
	SDL_Event ev = e;
	move_event(&ev, x(), y());
	const int max_widget = widgets_.size();
	int widget = offset_;
	int p_used = 0;
	int max_p = axis() == HORIZONTAL ? width() : height();
	while(widget < max_widget && p_used < max_p) {
		int d_used = axis() == HORIZONTAL ? widgets_[widget]->width() : widgets_[widget]->height();
		claimed = widgets_[widget]->process_event(ev);
		p_used += d_used;
		++widget;
		if(claimed) {
			break;
		}
		if(axis() == HORIZONTAL) {
			move_event(&ev, d_used, 0);
		} else {
			move_event(&ev, 0, d_used);
		}
	}
	return claimed;

}

void scroll_button::prepare_draw() const {
	bool can_change;
	if(amt_ > 0) {
		can_change = cont_->offset() < cont_->max_offset();
	} else if(amt_ < 0) {
		can_change = cont_->offset() > 0;
	} else {
		can_change = false;
	}
	if(!can_change && is_enabled()) {
		set_state(DISABLED);
	} else if(can_change && !is_enabled()) {
		set_state(NORMAL);
	}

	button_widget::prepare_draw();
}

void scroll_button::clicked()
{
	cont_->scroll(amt_);
}

void menu_option::construct_interface(const std::string& text, const std::string& frame,
				      const SDL_Color& color, int font_size)
{
	label_.reset(new label(text, color, font_size));
	label_ = frame_manager::make_frame(label_, frame);
	label_->set_loc(x(), y());
	//set_dim(label_->width(), label_->height());
}

void menu_option::inner_set_loc(int nx, int ny)
{
	skinned_widget::inner_set_loc(nx,ny);
	label_->set_loc(x(), y());
}

void menu_option::inner_set_dim(int nw, int nh)
{
	skinned_widget::inner_set_dim(nw,nh);
	label_->set_dim(width(), height());
}

void menu_option::inner_draw() const
{
	label_->draw();
}

void menu_widget::add_option(const std::string& opt, int state)
{
	menu_option_ptr opt_widget(new menu_option(opt, option_text_frame_,
						   color_, font_size_));
	options_[state] = opt_widget;
	for(std::map<int,std::string>::iterator i = option_skins_.begin();
	    i != option_skins_.end(); ++i) {
		opt_widget->add_skin(i->second, i->first);
	}
	rebuild_options();
}

void menu_widget::remove_option(int state)
{
	options_.erase(state);
	rebuild_options();
}

void menu_widget::add_option_skin(const std::string& skin, int state)
{
	option_skins_[state] = skin;
	for(iterator i = options_.begin(); i != options_.end(); ++i) {
		i->second->add_skin(skin, state);
	}
}

void menu_widget::remove_option_skin(int state) {
	option_skins_.erase(state);
	for(iterator i = options_.begin(); i != options_.end(); ++i) {
		i->second->remove_skin(state);
	}
}

bool menu_widget::is_option_enabled(int opt) const {
	return options_.find(opt)->second->state() != menu_option::DISABLED;
}
void menu_widget::set_option_enabled(int opt, bool enabled) {
	if(is_option_enabled(opt) == enabled) {
		return;
	}
	if(enabled) {
		options_[opt]->set_state(menu_option::NORMAL);
	} else {
		options_[opt]->set_state(menu_option::DISABLED);
	}
}

int menu_widget::find_option(const SDL_Event &e)
{
	const static int INT16_HIGH_BIT = 1 << (sizeof(Uint16)*8-1);
	const static int INT16_CAP = INT16_HIGH_BIT - 1;

	int mx, my;
	switch(e.type) {
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		mx = e.button.x;
		my = e.button.y;
		break;
	case SDL_MOUSEMOTION:
		mx = e.motion.x;
		my = e.motion.y;
		break;
	default:
		return -1;
	}

	if(mx > INT16_CAP) {
		mx &= INT16_CAP;
		mx -= INT16_HIGH_BIT;
	}
	if(my > INT16_CAP) {
		my &= INT16_CAP;
		my -= INT16_HIGH_BIT;
	}

	int ox, oy;
	get_options_loc(&ox, &oy);
	mx -= ox;
	my -= oy;

	int count = 0;
	for(const_iterator i = begin_options();  i != end_options(); ++i) {
		const menu_option_ptr opt = i->second;
		if(mx > opt->x() && mx < opt->x() + opt->width() &&
		   my > opt->y() && my < opt->y() + opt->height()) {
			return count;
		}
		++count;
	}
	return -1;
}

void menu_widget::update_key_selection(const SDL_Event &e)
{
	switch(e.type) {
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		key_selection_ = -1;
		break;
	case SDL_KEYDOWN:
		switch(e.key.keysym.sym) {
		case SDLK_DOWN:
		        {
				int start;
				if(key_selection_ < 0) {
					start = 0;
					key_selection_ = -1;
				} else {
					start = key_selection_;
				}
				while(true) {
					++key_selection_;
					if(key_selection_ >= options_.size()) {
						key_selection_ -= options_.size();
					}
					if(is_option_enabled(key_selection_) || key_selection_ == start) {
						break;
					}
				}
			}
			break;
		case SDLK_UP:
		        {
				int start;
				if(key_selection_ < 0) {
					start = options_.size()-1;
					key_selection_ = options_.size();
				} else {
					start = key_selection_;
				}
				while(true) {
					--key_selection_;
					while(key_selection_ < 0) {
						key_selection_ += options_.size();
					}
					if(is_option_enabled(key_selection_) || key_selection_ == start) {
						break;
					}
				}
			}
			break;
		default:
			break;
		}
	default:
		break;
	}
}

void menu_widget::inner_set_loc(int nx, int ny)
{
	const bool changed = nx != x() || ny != y();
	skinned_widget::inner_set_loc(nx,ny);
	if(changed) rebuild_options();
}

void menu_widget::inner_set_dim(int nw, int nh)
{
	const bool changed = nw != width() || nh != height();
	skinned_widget::inner_set_dim(nw,nh);
	if(changed) rebuild_options();
}

bool popup_menu_widget::is_enabled() const
{
	return state() != DISABLED;
}

void popup_menu_widget::set_enabled(bool enabled) {
	if(is_enabled() == enabled) {
		return;
	}
	if(enabled) {
		set_state(NORMAL);
	} else {
		set_state(DISABLED);
	}
}

void popup_menu_widget::rebuild_options() {
	phantom_dialog_ptr d(new phantom_dialog());
	d->set_padding(0);
	for(menu_widget::iterator i = begin_options(); i != end_options(); ++i) {
		menu_option_ptr opt = i->second;
		widget_ptr frame = frame_manager::make_frame(opt, option_frame_name_);
		d->add_widget(frame, dialog::MOVE_DOWN);
	}
	option_frame_ = frame_manager::make_frame(d, option_set_frame_name_);

	switch(alignment_) {
	case RIGHT:
		option_frame_->set_loc(x() - (option_frame_->width() - width()),
				       y() - option_frame_->height());
		break;
	case LEFT:
		option_frame_->set_loc(x(), y() - option_frame_->height());
		break;
	case CENTER:
		option_frame_->set_loc(x() - (option_frame_->width() - width())/2,
				       y() - option_frame_->height());
		break;
	}
}

void popup_menu_widget::finish_draw() const
{
	if(popped_out_ && option_frame_) {
		option_frame_->draw();
	}
	menu_widget::finish_draw();
}

void popup_menu_widget::get_options_loc(int *x, int *y)
{
	if(option_frame_) {
		*x = option_frame_->x();
		*y = option_frame_->y();
	} else {
		*x = 0;
		*y = 0;
	}
}

bool popup_menu_widget::grab_option_event(const SDL_Event& e)
{
	update_key_selection(e);
	int selected = find_option(e);
	bool grab = selected >= 0;

	if(selected < 0) {
		selected = key_selection();
		if(selected >= 0) {
			switch(e.type) {
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				switch(e.key.keysym.sym) {
				case SDLK_UP:
				case SDLK_DOWN:
				case SDLK_RETURN:
					grab = true;
					break;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
	}

	bool can_select =  selected >=0 && is_option_enabled(selected);

	if(can_select) {
		switch(e.type) {
		case SDL_MOUSEBUTTONDOWN:
			if(get_option(selected)->state() != menu_option::DISABLED) {
				opt_ready_ = selected;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			if(opt_ready_ == selected) {
				option_selected(selected);
				set_state(NORMAL);
				popped_out_ = false;
			}
			opt_ready_ = -1;
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			switch(e.key.keysym.sym) {
			case SDLK_RETURN:
				option_selected(selected);
				set_state(NORMAL);
				popped_out_ = false;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	for(iterator i = begin_options(); i != end_options(); ++i) {
		menu_option::STATE sel_state;
		if(!is_option_enabled(i->first)) {
			sel_state = menu_option::DISABLED;
		} else {
			if(i->first == selected) {
				if(i->first == opt_ready_) {
					sel_state = menu_option::CLICKED;
				} else if(opt_ready_ > 0) {
					sel_state = menu_option::NORMAL;
				} else {
					sel_state = menu_option::HIGHLIGHTED;
				}
			} else {
				sel_state = menu_option::NORMAL;
			}
		}
		i->second->set_state(sel_state);
	}

	return grab;
}

bool popup_menu_widget::handle_event(const SDL_Event& e)
{
	if(state() == DISABLED) {
		return false;
	}

	if(popped_out_ && grab_option_event(e)) {
		return true;
	}

	bool claimed = false;

	switch(e.type) {
	case SDL_MOUSEBUTTONDOWN:
		if(hit_me(e)) {
			ready_ = true;
			set_state(CLICKED);
		} else {
			set_state(NORMAL);
			set_popped_out(false);
		}
		break;
	case SDL_MOUSEBUTTONUP:
		if(hit_me(e) && ready_) {
			if(!popped_out_) {
				set_state(DEPRESSED_HIGHLIGHTED);
				set_popped_out(true);
			} else {
				set_state(HIGHLIGHTED);
				set_popped_out(false);
			}
			claimed = true;
		} else {
			set_state(NORMAL);
			set_popped_out(false);
		}
		ready_ = false;
		break;
	case SDL_MOUSEMOTION:
		if(hit_me(e)) {
			if(ready_) {
				set_state(CLICKED);
			} else {
				if(!popped_out_) {
					set_state(HIGHLIGHTED);
				} else {
					set_state(DEPRESSED_HIGHLIGHTED);
				}
			}
		} else {
			if(!popped_out_) {
				set_state(NORMAL);
			} else {
				set_state(DEPRESSED);
			}
		}
		break;
	case SDL_KEYDOWN:
		if(has_hotkey() && e.key.keysym.sym == hotkey().sym && e.key.keysym.mod == hotkey().mod) {
			set_state(CLICKED);
		}
		break;
	case SDL_KEYUP:
		if(has_hotkey() && e.key.keysym.sym == hotkey().sym && e.key.keysym.mod == hotkey().mod) {
			if(!popped_out_) {
				set_state(DEPRESSED);
				set_popped_out(true);
			} else {
				set_state(NORMAL);
				set_popped_out(false);
			}
			claimed = true;
		}
		break;
	default:
		break;
	}
	return claimed;

}

void rollin_menu_widget::rebuild_options() {
	phantom_dialog_ptr d(new phantom_dialog());
	d->set_padding(0);
	for(menu_widget::iterator i = begin_options(); i!=end_options();++i) {
		menu_option_ptr opt = i->second;
		widget_ptr frame = frame_manager::make_frame(opt, option_frame_name());
		frame->set_dim(width(), height()/num_options());
		d->add_widget(frame, dialog::MOVE_DOWN);
	}
	frame_ptr option_frame = frame_manager::make_frame(d, option_set_frame_name());
	option_frame->set_loc(x(), y());
	set_option_frame(option_frame);
}

bool rollin_menu_widget::handle_event(const SDL_Event& e)
{
	if(state() == DISABLED) {
		return false;
	}
	if(popped_out() && grab_option_event(e)) {
		return true;
	}

	bool claimed = false;

	switch(e.type) {
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEMOTION:
		if(hit_me(e)) {
			set_state(ROLLED_OVER);
			set_popped_out(true);
		} else {
			set_state(NORMAL);
			set_popped_out(false);
		}
		break;
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		if(has_hotkey() && e.key.keysym.sym == hotkey().sym && e.key.keysym.mod == hotkey().mod) {
			if(!popped_out()) {
				set_state(ROLLED_OVER);
				set_popped_out(true);
			} else {
				set_state(NORMAL);
				set_popped_out(false);
			}
			claimed = true;
		}
		break;
	default:
		break;
	}
	return claimed;
}

void framed_dialog::handle_draw() const
{
	if(!frame_) {
		prepare_draw();
		inner_draw();
		finish_draw();
		return;
	}
	if(nested_draw_) {
		inner_draw();
	} else {
		prepare_draw();
		nested_draw_ = true;
		try {
			frame_->draw();
		} catch(...) {
			nested_draw_ = false;
			throw;
		}
		nested_draw_ = false;
		finish_draw();
	}
}

void framed_dialog::handle_draw_children() const {
	for(dialog::child_iterator i = begin_children(); i != end_children(); ++i)
	{
		(*i)->draw();
	}
}

void framed_dialog::prepare_draw() const {
	glPushMatrix();
	glTranslatef(x(), y(), 0);
}

void framed_dialog::finish_draw() const {
	glPopMatrix();
}

void framed_dialog::inner_draw() const {
	handle_draw_children();
}


void framed_dialog::set_dim(int w, int h)
{
	if(!frame_) {
		inner_set_dim(w,h);
		return;
	}
	if(nested_set_dim_) {
		inner_set_dim(w,h);
	} else {
		nested_set_dim_ = true;
		try {
			frame_->set_dim(w,h);
		} catch(...) {
			nested_set_dim_ = false;
			throw;
		}
		nested_set_dim_ = false;
	}
}

void framed_dialog::inner_set_dim(int w, int h) {
	dialog::set_dim(w,h);
}

void framed_dialog::set_loc(int x, int y)
{
	if(!frame_) {
		inner_set_loc(x,y);
		return;
	}
	if(nested_set_loc_) {
		inner_set_loc(x,y);
	} else {
		nested_set_loc_ = true;
		try {
			frame_->set_loc(x,y);
		} catch(...) {
			nested_set_loc_ = false;
			throw;
		}
		nested_set_loc_ = false;
	}
}

void framed_dialog::inner_set_loc(int x, int y) {
	dialog::set_loc(x,y);
}

void phantom_dialog::adapt_size()
{
	int dlg_w = width();
	int dlg_h = height();

	for(child_iterator i = begin_children(); i != end_children(); ++i) {
		const widget_ptr w = *i;
		const int wid_brx = w->x() + w->width();
		const int wid_bry = w->y() + w->height();

		if(wid_brx > dlg_w) {
			dlg_w = wid_brx;
		}
		if(wid_bry > dlg_h) {
			dlg_h = wid_bry;
		}
	}

	if(dlg_w != width() || dlg_h != height()) {
		set_dim(dlg_w, dlg_h);
	}
}

dialog& phantom_dialog::add_widget(widget_ptr w, MOVE_DIRECTION dir)
{
	dialog& ret = dialog::add_widget(w, dir);
	adapt_size();
	return ret;
}

dialog& phantom_dialog::add_widget(widget_ptr w, int x, int y, MOVE_DIRECTION dir)
{
	dialog& ret = dialog::add_widget(w, x, y, dir);
	adapt_size();
	return ret;
}

void phantom_dialog::remove_widget(widget_ptr w)
{
	dialog::remove_widget(w);
	adapt_size();
}

void phantom_dialog::replace_widget(widget_ptr w_old, widget_ptr w_new)
{
	dialog::replace_widget(w_old, w_new);
	adapt_size();
}

void phantom_dialog::clear() {
	dialog::clear();
	adapt_size();
}

}
