#include <map>
#include <set>
#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "filesystem.hpp"
#include "formula.hpp"
#include "texture.hpp"
#include "font.hpp"
#include "wml_node.hpp"
#include "wml_parser.hpp"
#include "raster.hpp"

#include "frame.hpp"

namespace gui {
namespace frame_manager {

std::map<std::string,fb_ptr> frame_builders;
std::set<std::string> missing_frames;

void load_frame_data(const std::string& name);

frame_ptr make_frame(widget_ptr w, const std::string& name, key_mapper_ptr keys) {
	std::map<std::string,fb_ptr>::const_iterator i = frame_builders.find(name);
	if(i == frame_builders.end()) {
		std::set<std::string>::const_iterator j = missing_frames.find(name);
		if(j == missing_frames.end()) {
			load_frame_data(name);
			i = frame_builders.find(name);
		}
	}
	fb_ptr builder;
	if(i == frame_builders.end()) {
		missing_frames.insert(name);
	} else {
		builder = i->second;
	}
	frame_ptr ret(new frame(w, builder, keys));
	
	return ret;
}

frame_ptr make_frame(widget_ptr w, const std::string& name) {
	key_mapper_ptr keys(new key_mapper());
	return make_frame(w, name, keys);
}


void flush_frame(const std::string& name) { frame_builders.erase(name); };
void flush_frames() { frame_builders.clear(); }

/* internal */
fdb_ptr make_fdb(const wml::node_ptr& node) { 
	fdb_ptr ret;

	const std::string& type = node->name();

	if(type == "texture") {
		ret = fdb_ptr(new texture_fdb());
	} else if(type == "rect") {
		ret = fdb_ptr(new rect_fdb());
	} else if(type == "text") {
		ret = fdb_ptr(new text_fdb());
	} else {
		std::cerr << "Unknown draw object \""<<type<<"\"\n";
	}
	ret->init_from_wml(node);
	return ret;
}

void parse_predraw(wml::const_node_ptr predraw, fb_ptr builder) 
{
	for(wml::node::const_all_child_iterator i = predraw->begin_children();
	    i != predraw->end_children(); ++i) {
		fdb_ptr fdb = make_fdb(*i);
		if(!fdb.get()) continue;
		builder->add_predraw(fdb);
	}
}

void parse_postdraw(wml::const_node_ptr postdraw, fb_ptr builder) 
{
	for(wml::node::const_all_child_iterator i = postdraw->begin_children();
	    i != postdraw->end_children(); ++i) {
		fdb_ptr fdb = make_fdb(*i);
		if(!fdb.get()) continue;
		builder->add_postdraw(fdb);
	}
}

void load_frame_data(const std::string& name) {
	const std::string filename = "images/frames/" + name + ".cfg";
	
	wml::const_node_ptr frame_wml;
	try {
		frame_wml = wml::parse_wml(sys::read_file(filename));
	} catch(...) {
		std::cerr << "Errors parsing frame file "<<filename<<"\n";
		return;
	}
	
	fb_ptr builder(new frame_builder()); 

	for(wml::node::const_attr_iterator at_iter = frame_wml->begin_attr();
	    at_iter != frame_wml->end_attr(); ++at_iter) {
		builder->set_attr(at_iter->first, formula_ptr(new game_logic::formula(at_iter->second)));
	}


	wml::const_node_ptr predraw = frame_wml->get_child("predraw");
	if(predraw) {
		parse_predraw(predraw, builder);
	}
	wml::const_node_ptr postdraw = frame_wml->get_child("postdraw");
	if(postdraw) {
		parse_postdraw(postdraw, builder);
	}

	frame_builders[name] = builder;
}


void frame_builder::initialise_frame(frame* frame) const
{
	const key_mapper_ptr keys = frame->get_keys();

	for(std::map<std::string,formula_ptr>::const_iterator i = formula_map_.begin();
	    i != formula_map_.end(); ++i) {
		const int value = i->second->execute(*keys).as_int();
		if(i->first == "border_left") {
			frame->set_border_left(value);
		} else if(i->first == "border_right") {
			frame->set_border_right(value);
		} else if(i->first == "border_top") {
			frame->set_border_top(value);
		} else if(i->first == "border_bottom") {
			frame->set_border_bottom(value);
		}
	}
	for(std::vector<fdb_ptr>::const_iterator i = predraw_.begin();
	    i != predraw_.end(); ++i) {
		frame->add_predraw((*i)->make_fdo(keys));
	}
	for(std::vector<fdb_ptr>::const_iterator i = postdraw_.begin();
	    i != postdraw_.end(); ++i) {
		frame->add_postdraw((*i)->make_fdo(keys));
	}
}

void texture_fdo::draw() const
{
  	graphics::push_clip(r_);
	if(!tile_) {
		graphics::blit_texture(texture_, r_.x,r_.y,r_.w,r_.h);
	} else {
		const int tw = texture_.width();
		const int th = texture_.height();
		
		const int tx_max = r_.x + r_.w;
		const int ty_max = r_.y + r_.h;
		
		for(int ty = r_.y; ty < ty_max; ty += th) {
			for(int tx = r_.x; tx < tx_max; tx += tw) {
				graphics::blit_texture(texture_, tx, ty);
			}
		}
	}
  	graphics::pop_clip();
}

void rect_fdo::draw() const
{
	if(fill_) {
		graphics::draw_rect(r_, color_, alpha_);
	} else {
		graphics::draw_hollow_rect(r_, color_, alpha_);
	}
}

void text_fdo::draw() const
{
	graphics::blit_texture(texture_, x_, y_);
}

void frame_draw_builder::init_from_wml(const wml::const_node_ptr& node) {
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		set_attr(i->first, i->second);
	}
}	

void frame_draw_builder::set_attr(const std::string& name, const std::string& value)
{
	switch(get_attr_type(name)) {
	case ATTR_UNKNOWN:
		std::cerr << "Warning: unknown attribute \""<<name<<"\" in frame file.\n";
		break;
	case ATTR_STRING:
		string_attrs_[name] = value;
		break;
	case ATTR_BOOL:
	case ATTR_INT:
		formula_attrs_[name] = formula_ptr(new game_logic::formula(value));
		break;
	default:
		std::cerr << "Error: invalid attribute type returned\n";
		assert(false);
		break;
	}
}

formula_ptr frame_draw_builder::get_formula_attr(const std::string& name) const
{
	std::map<std::string, formula_ptr>::const_iterator i =
		formula_attrs_.find(name);
	if(i != formula_attrs_.end()) {
		return i->second;
	}
	return formula_ptr();
}

int frame_draw_builder::get_int_attr(const std::string& name, 
				     const key_mapper_ptr &keys, int deflt) const
{
	formula_ptr fptr = get_formula_attr(name);
	int ret = deflt;
	if(fptr.get()) {
		ret= fptr->execute(*keys).as_int();
	}		
	return ret;
}

int frame_draw_builder::get_int_attr(const std::string& name,
				     const key_mapper_ptr& keys, int deflt,
				     int min, int max) const
{
	int ret = get_int_attr(name, keys, deflt);
	if(ret > max) {
		ret = max;
	} else if(ret < min) {
		ret = min;
	}
	return ret;
}

bool frame_draw_builder::get_bool_attr(const std::string& name,
				       const key_mapper_ptr& keys, 
				       bool deflt) const
{
	formula_ptr fptr = get_formula_attr(name);
	if(fptr.get()) {
		return fptr->execute(*keys).as_bool();
	}
	return deflt;
}


std::string frame_draw_builder::get_string_attr(const std::string& name,
						const key_mapper_ptr &keys,
						const std::string& deflt) const {	
	std::map<std::string, std::string>::const_iterator i =
		string_attrs_.find(name);
	if(i != string_attrs_.end()) {
		return i->second;
	}
	return deflt;
}


fdo_ptr texture_fdb::make_fdo(const key_mapper_ptr& keys) const
{
	int x = get_int_attr("x", keys, 0);
	int y = get_int_attr("y", keys, 0);
	int w = get_int_attr("w", keys, 0);
	int h = get_int_attr("h", keys, 0);
	std::string texture = get_string_attr("file", keys, "");
	bool tile = get_bool_attr("tile", keys, true);

	return fdo_ptr(new texture_fdo(x,y,w,h,texture,tile));
}

frame_draw_builder::attr_type texture_fdb::get_attr_type(const std::string& name) const 
{
	if(name == "x" || name == "y" || name == "w" || name == "h") {
		return ATTR_INT;
	} else if(name == "file") {
		return ATTR_STRING;
	} else if(name == "tile") {
		return ATTR_BOOL;
	}
	return ATTR_UNKNOWN;
}

fdo_ptr rect_fdb::make_fdo(const key_mapper_ptr& keys) const
{
	int x = get_int_attr("x", keys, 0);
	int y = get_int_attr("y", keys, 0);
	int w = get_int_attr("w", keys, 0);
	int h = get_int_attr("h", keys, 0);
	bool fill = get_bool_attr("fill", keys, true);
	SDL_Color color;
	color.r = get_int_attr("r", keys, 0, 0, 255);
	color.g = get_int_attr("g", keys, 0, 0, 255);
	color.b = get_int_attr("b", keys, 0, 0, 255);
	color.unused = 255;
	int alpha = get_int_attr("a", keys, 255, 0, 255);
	return fdo_ptr(new rect_fdo(x,y,w,h,fill,color,alpha));
}

frame_draw_builder::attr_type rect_fdb::get_attr_type(const std::string& name) const
{
	if(name == "x" || name == "y" || name == "w" || name == "h" ||
	   name == "r" || name == "g" || name == "b" || name == "a") {
		return ATTR_INT;
	} else if(name == "fill") {
		return ATTR_BOOL;
	}
	return ATTR_UNKNOWN;
}

fdo_ptr text_fdb::make_fdo(const key_mapper_ptr& keys) const
{
	int x = get_int_attr("x", keys, 0);
	int y = get_int_attr("y", keys, 0);
	int size = get_int_attr("size", keys, 0);
	std::string text = get_string_attr("text", keys, "");
	SDL_Color color;
	color.r = get_int_attr("r", keys, 0, 0, 255);
	color.g = get_int_attr("g", keys, 0, 0, 255);
	color.b = get_int_attr("b", keys, 0, 0, 255);
	color.unused = 255;
	return fdo_ptr(new text_fdo(x,y,size,color,text));
}

frame_draw_builder::attr_type text_fdb::get_attr_type(const std::string& name) const
{
	if(name == "x" || name == "y" || name == "size" ||
	   name == "r" || name == "g" || name == "b") {
		return ATTR_INT;
	} else if(name == "text") {
		return ATTR_STRING;
	}
	return ATTR_UNKNOWN;
}

void key_mapper::add_rect(const std::string& name, int x, int y, int w, int h) 
{
	SDL_Rect r;
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;

	boost::shared_ptr<rect_mapper> rect(new rect_mapper(r));
	rects_[name] = rect;
}
	
void key_mapper::remove_rect(const std::string& name) 
{
	rects_.erase(name);
}

variant key_mapper::get_value(const std::string& name) const
{
	std::map<std::string,boost::shared_ptr<rect_mapper> >::const_iterator i =
		rects_.find(name);
	if(i != rects_.end()) {
		return variant(i->second.get());
	}
	return variant(0);
}

rect_mapper::rect_mapper(const SDL_Rect& r)
	: x_(r.x), y_(r.y), w_(r.w), h_(r.h) {}

variant rect_mapper::get_value(const std::string& name) const
{
	if(name == "x") {
		return variant(x_);
	} else if(name == "y") {
		return variant(y_);
	} else if(name == "w") {
		return variant(w_);
	} else if(name == "h") {
		return variant(h_);
	}
	return variant(0);
}

} // namespace frame_manager
} // namespace gui
