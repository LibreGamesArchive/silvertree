#ifndef GUI_CORE_HPP_INCLUDED
#define GUI_CORE_HPP_INCLUDED

#include <set>
#include <vector>
#include "SDL.h"
#include "boost/shared_ptr.hpp"

#include "widget.hpp"
#include "dialog.hpp"
#include "frame.hpp"

namespace gui {

class phantom_widget: public widget {
public:
	void handle_draw() const {}
};

class delegate_widget: public widget {
public:
	delegate_widget(const widget* w) : w_(w) {}
	void handle_draw() const { w_->draw(); }
private:
	const widget* w_;
};

class phantom_dialog: public dialog {
public:
	phantom_dialog() : dialog(0, 0, 0, 0) { set_clear_bg(false); }
	virtual ~phantom_dialog() {}
	virtual dialog& add_widget(widget_ptr w, MOVE_DIRECTION dir=MOVE_DOWN);
	virtual dialog& add_widget(widget_ptr w, int x, int y,
	                MOVE_DIRECTION dir=MOVE_DOWN);
	virtual void remove_widget(widget_ptr w);
	virtual void replace_widget(widget_ptr w_old, widget_ptr w_new);
	virtual void clear();
private:
	void adapt_size();
};

typedef boost::shared_ptr<phantom_dialog> phantom_dialog_ptr;

class skinned_widget: public widget {
public:
	skinned_widget();
	virtual ~skinned_widget() {}
	int state() const { return state_; }
	void set_state(int state) const;
	int add_skin(const std::string& name, int state=-1);
	void remove_skin(int state);
	void set_loc(int x, int y);
	void set_dim(int w, int h);
protected:
	virtual void prepare_draw() const {}
	virtual void inner_draw() const {}
	virtual void finish_draw() const {}
	virtual void inner_set_loc(int x, int y);
	virtual void inner_set_dim(int w, int h);
private:
	void handle_draw() const;
	bool valid_frame() const;
	mutable int state_;
	std::vector<frame_ptr> frames_;
	std::set<int> cancelled_frames_;
	bool have_real_dim_, have_real_loc_;
	mutable bool nested_draw_;
	SDL_Rect real_dims_;
	widget_ptr inner_;
};

class button_widget: public skinned_widget {
public:
	enum STATE { NORMAL = 0, HIGHLIGHTED, CLICKED, DISABLED };

	button_widget() : ready_(false), has_hotkey_(false) {}
	void set_enabled(bool enabled);
	bool is_enabled() const;
	bool has_hotkey() { return has_hotkey_; }
	const SDL_keysym &hotkey() { return hotkey_; }
	void set_hotkey(SDLKey sym, SDLMod mod) { hotkey_.sym = sym; hotkey_.mod = mod, has_hotkey_ = true; }
	void remove_hotkey() { has_hotkey_ = false; }
protected:
	virtual ~button_widget() {}
	bool hit_me(const SDL_Event &e);
private:
	void do_click();
	bool handle_event(const SDL_Event &e);
	virtual void clicked() {}
	SDL_keysym hotkey_;
	bool ready_, has_hotkey_;
};

typedef boost::shared_ptr<button_widget> button_widget_ptr;

class scrolled_container : public widget {
public:
	typedef std::vector<widget_ptr>::iterator iterator;
	typedef std::vector<widget_ptr>::const_iterator const_iterator;

	scrolled_container() : offset_(0) {}

	int offset() const { return offset_; }
	int max_offset() const { return widgets_.size() - 1; }
	void scroll(int amount);
	void set_offset(int pos);
	void add_widget(widget_ptr p);
	void remove_widget(widget_ptr p);
	void clear() { widgets_.clear(); }
	const_iterator begin_children() const { return widgets_.begin(); }
	const_iterator end_children() const { return widgets_.end(); }
private:
	static void move_event(SDL_Event*e, int x, int y);
	void handle_draw() const;
	bool handle_event(const SDL_Event& e);
	int offset_;
	std::vector<widget_ptr> widgets_;
};

typedef boost::shared_ptr<scrolled_container> scrolled_container_ptr;


class scroll_button: public button_widget {
public:
	scroll_button(scrolled_container_ptr cont, int amount_per_click) 
		: cont_(cont), amt_(amount_per_click) {}
protected:
	void prepare_draw() const;
private:
	void clicked();
	scrolled_container_ptr cont_;
	int amt_;
};

class menu_option : public skinned_widget {
public:
	enum STATE { NORMAL, HIGHLIGHTED, CLICKED, DISABLED };
	menu_option(const std::string& text, const std::string& frame,
		    const SDL_Color& color, int font_size) 
	{
		construct_interface(text, frame, color, font_size);
	}
private:
	void inner_draw() const;
	void inner_set_loc(int x, int y);
	void inner_set_dim(int w, int h);
	void construct_interface(const std::string& text, const std::string& frame,
				 const SDL_Color& color, int font_size);

	widget_ptr label_;
};

typedef boost::shared_ptr<menu_option> menu_option_ptr;

class menu_widget : public skinned_widget {
public:
	typedef std::map<int,menu_option_ptr>::const_iterator const_iterator;
	typedef std::map<int,menu_option_ptr>::iterator iterator;

	menu_widget(const std::string& option_text_frame, const SDL_Color& color, int font_size) 
		: option_text_frame_(option_text_frame), font_size_(font_size),
		  color_(color), key_selection_(-1) {}
	virtual ~menu_widget() {}

	void add_option(const std::string& opt, int value);
	void remove_option(int value);
	void add_option_skin(const std::string& name, int state);
	void remove_option_skin(int state);
	bool is_option_enabled(int opt) const;
	void set_option_enabled(int opt, bool enabled);
	void set_hotkey(SDLKey sym, SDLMod mod) { has_hotkey_ = true; hotkey_.sym = sym; hotkey_.mod = mod; }
	void remove_hotkey() { has_hotkey_ = false; }
	const SDL_keysym& hotkey() const { return hotkey_; }
	bool has_hotkey() const { return has_hotkey_; }
protected:
	const_iterator begin_options() const { return options_.begin(); }
	const_iterator end_options() const { return options_.end(); }
	iterator begin_options() { return options_.begin(); }
	iterator end_options() { return options_.end(); }
	int num_options() const { return options_.size(); }

	menu_option_ptr get_option(int i) { return options_[i]; }
	void update_key_selection(const SDL_Event& e);
	int key_selection() const { return key_selection_; }
	void inner_set_loc(int x, int y);
	void inner_set_dim(int w, int h);

	virtual void option_selected(int option)=0;
	virtual void rebuild_options() =0;
	virtual void get_options_loc(int *x, int *y) =0;
	bool hit_me(const SDL_Event &e);
	int find_option(const SDL_Event &e);
private:
	std::map<int,std::string> option_skins_;
	std::map<int,menu_option_ptr> options_;
	const std::string option_text_frame_;
	int font_size_;
	const SDL_Color color_;
	int key_selection_;
	bool has_hotkey_;
	SDL_keysym hotkey_;
};

typedef boost::shared_ptr<menu_widget> menu_widget_ptr;

class popup_menu_widget : public menu_widget {
public:
	enum ALIGN { LEFT, CENTER, RIGHT };
	enum STATE { NORMAL=0, HIGHLIGHTED, DEPRESSED, DEPRESSED_HIGHLIGHTED, CLICKED, DISABLED };
	
	popup_menu_widget(const std::string& option_text_frame_name, const SDL_Color& color, int font_size, 
			  const std::string& option_frame_name, 
			  const std::string& option_set_frame_name) 
		: menu_widget(option_text_frame_name, color, font_size),
		  popped_out_(false), ready_(false), opt_ready_(-1), alignment_(LEFT),
		  option_frame_name_(option_frame_name),
		  option_set_frame_name_(option_set_frame_name) {}
	bool is_enabled() const;
	void set_enabled(bool enabled);
	bool popped_out() const { return popped_out_; }
	void set_popped_out(bool popped_out) { popped_out_ = popped_out; opt_ready_ = -1; }
protected:
	void rebuild_options();
	void finish_draw() const;
	void get_options_loc(int *x, int *y);
	const std::string& option_frame_name() { return option_frame_name_; }
	const std::string& option_set_frame_name() { return option_set_frame_name_; }
	void set_option_frame(frame_ptr frame) { option_frame_ = frame; }
	bool grab_option_event(const SDL_Event &e);
private:
	bool handle_event(const SDL_Event& e);

	bool popped_out_, ready_;
	int opt_ready_;
	enum ALIGN alignment_;
	const std::string option_frame_name_;
	const std::string option_set_frame_name_;
	widget_ptr option_frame_;
};

class rollin_menu_widget : public popup_menu_widget {
public:
	enum STATE { NORMAL=0, ROLLED_OVER, DISABLED };

	rollin_menu_widget(const std::string& option_text_frame_name, const SDL_Color& color, int font_size,
			   const std::string& option_frame_name, 
			   const std::string& option_set_frame_name)
		: popup_menu_widget(option_text_frame_name, color, font_size, 
				    option_frame_name, option_set_frame_name) {}
protected:
	void rebuild_options();
private:
	bool handle_event(const SDL_Event &e);
};

class framed_dialog: public dialog {
public:
	void set_frame(frame_ptr frame) { frame_ = frame; }
	const frame_ptr get_frame() { return frame_; }
	void set_loc(int x, int y);
	void set_dim(int w, int h);
protected:
	framed_dialog(int x, int y, int w, int h) 
		: dialog(x,y,w,h), 
		  nested_draw_(false), nested_set_dim_(false), nested_set_loc_(false) {}
	virtual ~framed_dialog() {}
	void handle_draw_children() const;
private:
	void handle_draw() const;
	virtual void inner_set_dim(int w, int h);
	virtual void inner_set_loc(int x, int y);
	virtual void prepare_draw() const;
	virtual void inner_draw() const;
	virtual void finish_draw() const;

	frame_ptr frame_;
	mutable bool nested_draw_, nested_set_dim_, nested_set_loc_;
};

}


#endif
