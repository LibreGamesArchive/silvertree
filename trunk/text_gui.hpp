#ifndef TEXT_GUI_HPP_INCLUDED
#define TEXT_GUI_HPP_INCLUDED

#include <vector>
#include <map>
#include <string>
#include <iterator>

#include "SDL.h"
#include "boost/shared_ptr.hpp"

#include "gui_core.hpp"
#include "raster.hpp"
#include "unicode.hpp"

namespace gui {
namespace text {

/* push the current unicode state and enable unicode */
void enter_text_mode();
/* restore the previous unicode state overwritten by enter_text_mode */
void leave_text_mode();

class editable_text {
public:
	typedef std::vector<Uint32>::iterator iterator;
	typedef std::vector<Uint32>::const_iterator const_iterator;
	typedef std::vector<Uint32>::reverse_iterator reverse_iterator;
	typedef std::vector<Uint32>::const_reverse_iterator const_reverse_iterator;
	
	editable_text(const std::string& text) 
		: mark_set_(false), text_()
	{ 
		text_ = utf8_to_utf32(text);
		caret_ = text.size();
		enter_text_mode(); 
	}
	virtual ~editable_text() { leave_text_mode(); }
	void set_text(const std::string& text) { text_ = utf8_to_utf32(text); }
	std::string text() const { return utf32_to_utf8(text_); }
	bool has_mark_set() const { return mark_set_; }
	void set_mark(const iterator& x) { mark_ = int_from_itor(x); mark_set_ = true; }
	void clear_mark() { mark_set_ = false; }
	iterator mark() { return itor_from_int(mark_); }
	const_iterator mark() const { return itor_from_int(mark_); }
	void set_caret(const iterator& x) { caret_ = int_from_itor(x); }
	iterator caret() { return itor_from_int(caret_); }
	int caret_index() const { return caret_; }
	int mark_index() const { return mark_; }
	bool has_selection() const { return has_mark_set(); }
	void get_selection(int *start, int *end) const;
	const_iterator caret() const { return itor_from_int(caret_); }
	const_iterator begin() const { return text_.begin(); }
	const_iterator end() const { return text_.end(); }
	iterator begin() { return text_.begin(); }
	iterator end() { return text_.end(); }
	const_reverse_iterator rbegin() const { return text_.rbegin(); }
	const_reverse_iterator rend() const { return text_.rend(); }
	reverse_iterator rbegin() { return text_.rbegin(); }
	reverse_iterator rend() { return text_.rend(); }
	void clear() { text_.clear(); caret_ = 0; mark_set_ = false; }
	iterator erase(iterator i1, iterator i2);
	void insert(Uint32 letter);
	void undo();
	void redo();
private:	
	iterator itor_from_int(const int i)
	{ 
		iterator r= text_.begin(); 
		std::advance(r, i); 
		return r; 
	}
	int int_from_itor(iterator r) { 
		return std::distance(text_.begin(), r); 
	}
	const_iterator itor_from_int(const int i) const 
	{ 
		const_iterator r= text_.begin(); 
		std::advance(r, i); 
		return r; 
	}
	int int_from_itor(const_iterator r) const { 
		return std::distance(text_.begin(), r); 
	}

	struct edit_state {
		std::vector<Uint32> text;
		int caret, mark;
		bool mark_set;
	};

	void store_undo_info(std::vector<edit_state>& st);
	void load_undo_info(std::vector<edit_state>& st);

	bool mark_set_;
	std::vector<Uint32> text_;
	int caret_, mark_;
	std::vector<edit_state> undo_, redo_;
};

typedef boost::shared_ptr<editable_text> editable_text_ptr;

class sym_equal_p {
public:
	bool operator() (const SDL_keysym& s1, const SDL_keysym& s2) const {
		/* do this because sym.unicode is not the 
		   same for press and release events */
		if(s1.sym == s2.sym) {
			return s1.mod < s2.mod;
		}
		return s1.sym < s2.sym;
	}
};

class key_table {
public:
	enum logical_key {
		LK_NONE = 0,
		LK_CURSOR_FORWARD,
		LK_CURSOR_BACKWARD,
		LK_CURSOR_FORWARD_WORD,
		LK_CURSOR_BACKWARD_WORD,
		LK_CURSOR_FORWARD_LINE,
		LK_CURSOR_BACKWARD_LINE,
		LK_DELETE_FORWARD,
		LK_DELETE_BACKWARD,
		LK_DELETE_FORWARD_WORD,
		LK_DELETE_BACKWARD_WORD,
		LK_DELETE_FORWARD_LINE,
		LK_DELETE_BACKWARD_LINE,
		LK_DELETE_ALL,
		LK_SELECT_FORWARD,
		LK_SELECT_BACKWARD,
		LK_SELECT_FORWARD_WORD,
		LK_SELECT_BACKWARD_WORD,
		LK_SELECT_FORWARD_LINE,
		LK_SELECT_BACKWARD_LINE,
		LK_SELECT_ALL,
		LK_UNDO,
		LK_REDO
	};
	typedef std::map<SDL_keysym, logical_key>::iterator iterator;
	typedef std::map<SDL_keysym, logical_key>::const_iterator const_iterator;

	const_iterator begin_bindings() { return bindings_.begin(); }
	const_iterator end_bindings() { return bindings_.end(); }

	std::vector<SDL_keysym> bindings(logical_key k) const;
	void set_binding(const SDL_keysym& sym, const logical_key k) { bindings_[sym] = k; }
	void clear_binding(const SDL_keysym& sym) { bindings_.erase(sym); }
	void clear() { bindings_.clear(); }
	logical_key key(const SDL_keysym& sym) const;
private:
	std::map<SDL_keysym,logical_key,sym_equal_p> bindings_;
};	

typedef boost::shared_ptr<key_table> key_table_ptr;

/* find a key table by identifier */
const key_table_ptr get_key_table(int table);
/* set a binding for a particular table */
void set_key_binding(int table, SDLKey key, SDLMod mod, const key_table::logical_key k);
/* clear a binding for a given table */
void clear_key_binding(int table, SDLKey key, SDLMod mod);
/* wipe a key table */
void clear_key_table(int table);

/* prepare the key tables */
void init();

/* class encapsulating a particular operation on an editable_text */
class edit_operation {
public:
	virtual void operator()(editable_text &buf) =0;
	virtual ~edit_operation() {}
protected:
	editable_text::iterator unconst(editable_text& b, const editable_text::const_iterator& ci) {
		editable_text::iterator i = b.begin();
		std::advance(i, std::distance<editable_text::const_iterator>(b.begin(), ci));
		return i;
	}
};

typedef boost::shared_ptr<edit_operation> edit_operation_ptr;

class editor {
public:
	virtual bool process_event(const SDL_Event &e) =0;
	virtual ~editor() {}
};

typedef boost::shared_ptr<editor> editor_ptr;

class key_filter {
public:
	virtual ~key_filter() {}
	virtual bool matches(Uint32 ch) =0;
	std::vector<Uint32> filter(const std::vector<Uint32>& input);
};

typedef boost::shared_ptr<key_filter> key_filter_ptr;

class null_filter: public key_filter {
public:
	bool matches(Uint32 ch) { return false; }
};

class key_set_filter: public key_filter {
public:
	bool matches(Uint32 ch);
	void add_key(Uint32 ch) { chars_.insert(chars_.begin(), ch); }
	void add_key(char ch) { add_key(static_cast<Uint32>(ch)); }
	void remove_key(Uint32 ch) { chars_.erase(ch); }
	void clear() { chars_.clear(); }
private:
	std::set<Uint32> chars_;
};

typedef boost::shared_ptr<key_set_filter> key_set_filter_ptr;

class key_editor : public editor {
public:
	typedef std::map<key_table::logical_key,edit_operation_ptr>::iterator iterator;
	typedef std::map<key_table::logical_key,edit_operation_ptr>::const_iterator const_iterator;

	const static int DEFAULT_REPEAT_RATE = 50; /* ms */
	const static int DEFAULT_REPEAT_DELAY = 200;
	const static bool DEFAULT_REPEAT_ENABLED = true; 

	key_editor(int table, editable_text_ptr buf, 
		   const key_filter_ptr filter = key_filter_ptr(new null_filter()))
		: buf_(buf), kt_(get_key_table(table)),
		  repeat_rate_(DEFAULT_REPEAT_RATE),
		  repeat_delay_(DEFAULT_REPEAT_DELAY), 
		  repeat_enabled_(DEFAULT_REPEAT_ENABLED),
		  filter_(filter)
	{
		fill_function_table();
	}

	bool process_event(const SDL_Event &e);
	void handle_logical_key_press(const key_table::logical_key k);
	void handle_unicode_press(const Uint32 unicode);

	void start_key_repeating(const SDL_keysym& sym);
	void finish_key_repeating(const SDL_keysym& sym);
	void clear_key_repeating();

	void set_key_repeat_enabled(bool enabled) { repeat_enabled_ = enabled; }
	void set_key_repeat_rate(Uint32 rate) { repeat_rate_ = rate; }
	void set_key_repeat_delay(Uint32 delay) { repeat_delay_ = delay; }
	bool is_key_repeat_enabled() { return repeat_enabled_; }
	Uint32 get_key_repeat_rate() { return repeat_rate_; }
	Uint32 get_key_repeat_delay() { return repeat_delay_; }
private:
	void fill_function_table();
	
	editable_text_ptr buf_;
	/* the key table to use to interpret special keys */
	key_table_ptr kt_;
	/* a set of functions that change the editable_text */
	std::map<key_table::logical_key,edit_operation_ptr> functions_;
	/* the number of ms between the n and n+1 key repeats for n >1 */
	Uint32 repeat_rate_;
	/* the number of ms between the 1 and 2 key repeats */
	Uint32 repeat_delay_;
	/* whether key repeat is enabled */
	bool repeat_enabled_;
	/* the timer involved */
	SDL_TimerID timer_id_;
	/* a stack of keys held down in the order they were pressed */
	std::vector<SDL_keysym> repeating_key_;
	/* a filter of keys that should not be accepted */
	key_filter_ptr filter_;
};

/* this class is WIP */
class mouse_editor : public editor {
public:
	bool process_event(const SDL_Event& e) { return false; }
};

} /* namespace text */

class text_box : public skinned_widget {
public:
	enum STATE { NORMAL =0, HIGHLIGHTED, SELECTED, DISABLED };
	const static int DEFAULT_KEYTABLE = 0;

	text_box(const std::string& initial_text = "",
		 const text::key_filter_ptr filter = text::key_filter_ptr(new text::null_filter()),
		 int font_size =14, 
		 int key_table = DEFAULT_KEYTABLE,
		 const SDL_Color& color = graphics::color_white(),
		 const SDL_Color& caret_fg = graphics::color_black(),
		 const SDL_Color& caret_bg = graphics::color_white(),
		 const SDL_Color& selection_fg = graphics::color_white(),
		 const SDL_Color& selection_bg = graphics::color_blue(),
		 bool opaque_caret = true,
		 bool opaque_selection = true) 
		: edit_(new text::editable_text(initial_text)), 
		  font_size_(font_size), color_(color),
		  caret_fg_(caret_fg), caret_bg_(caret_bg),
		  selection_fg_(selection_fg), selection_bg_(selection_bg),
		  opaque_caret_(opaque_caret), opaque_selection_(opaque_selection),
		  cached_text_(""), cached_state_(NORMAL)
	{
		text::editor_ptr ek(new text::key_editor(key_table, edit_, filter));
		text::editor_ptr em(new text::mouse_editor());
		editors_.push_back(ek);
		editors_.push_back(em);
	}

	void set_color(const SDL_Color &color) { color_ = color; }
	void set_font_size(int size) { font_size_ = size; }
	const SDL_Color& color() const { return color_; }
	int font_size() const { return font_size_; }
	void set_text(const std::string& text) { edit_->set_text(text); }
	std::string text() { return edit_->text(); }
protected:
	bool handle_event(const SDL_Event &e);
	void inner_draw() const;
	void recalculate_texture() const;
private:
	text::editable_text_ptr edit_;
	int font_size_;
	SDL_Color color_, caret_fg_, caret_bg_, selection_fg_, selection_bg_;
	bool opaque_caret_, opaque_selection_;
	std::vector<text::editor_ptr> editors_;
	mutable graphics::texture texture_;
	mutable std::string cached_text_;
	mutable STATE cached_state_;
};

typedef boost::shared_ptr<text_box> text_box_ptr;

}/*namespace gui */

#endif

