#include <iostream>
#include <string>
#include <vector>

#include "frame.hpp"
#include "raster.hpp"
#include "surface_cache.hpp"

namespace gui {
namespace frame_manager {
 
frame::frame(widget_ptr base, fb_ptr builder, key_mapper_ptr keys) 
	: base_(base), basic_(true), builder_(builder), keys_(keys)
{
	border_l_ = 0;
	border_r_ = 0;
	border_t_ = 0;
	border_b_ = 0;

	const SDL_Color fg = { 0xFF,0xFF,0xFF,0xFF };
	const SDL_Color bg = { 0x00,0x00,0x00,0xFF };
	fg_ = fg;
	bg_ = bg;
	fixed_width_ = false;

	calculate_loc();
	calculate_dim();
	rebuild_frame();
}
				  
void frame::handle_draw() const
{
	if(basic_) {
		SDL_Rect rect;
		rect.x = x();
		rect.y = y();
		rect.w = width();
		rect.h = height();
		graphics::draw_rect(rect, bg_);
		base_->draw();
		graphics::draw_hollow_rect(rect, fg_);
		return;
	} 

	for(std::vector<fdo_ptr>::const_iterator i = predraw_.begin(); i != predraw_.end(); ++i) {
		(*i)->draw();
	}

	base_->draw();

	for(std::vector<fdo_ptr>::const_iterator i = postdraw_.begin(); i != postdraw_.end(); ++i) {
		(*i)->draw();
	}
}

void frame::rebuild_frame() {
	predraw_.clear();
	postdraw_.clear();
	if(builder_.get()) {
		keys_->add_rect("self", base_->x(), base_->y(), base_->width(), base_->height());
		builder_->initialise_frame(this);
		basic_ = false;
	} else {
		basic_ = true;
	}
}

void frame::handle_event(const SDL_Event &e)
{
	base_->process_event(e);
}

void frame::set_loc(int x, int y) 
{
	const int bx = x+border_l_;
	const int by = y+border_t_;

	base_->set_loc(bx, by);
	calculate_loc();

	rebuild_frame();
}

void frame::calculate_loc() {
	widget::set_loc(base_->x() - border_l_, base_->y() - border_t_);
}

void frame::set_dim(int w, int h)
{
	const int bw = w - border_width();
	const int bh = h - border_height();

	base_->set_dim(bw, bh);
	calculate_dim();
	rebuild_frame();
}

void frame::calculate_dim()
{
	widget::set_dim(base_->width() + border_width(), 
			base_->height() + border_height());
	std::cerr << "Frame has size "<<width() <<"x"<<height()<<"\n";
}

void frame::add_predraw(fdo_ptr fdo) {
	predraw_.push_back(fdo);
}
void frame::add_postdraw(fdo_ptr fdo) {
	postdraw_.push_back(fdo);
}

void frame::add_key_set(const std::string& name, int x, int y, int w, int h) {
	keys_->add_rect(name, x, y, w, h);
	rebuild_frame();
}

}
}
