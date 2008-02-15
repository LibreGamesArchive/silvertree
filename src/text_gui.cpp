#include <stack>
#include <algorithm>
#include <iterator>
#include "SDL_ttf.h"

#include "font.hpp"
#include "foreach.hpp"
#include "text_gui.hpp"
#include "userevents.h"

namespace gui {
namespace text {

namespace {
/* a stack of values for unicode enablement */
std::stack<int> unicode_stack;

/* a mapping of int (#defines or enums) to key tables */
std::map<int, boost::shared_ptr<key_table> > key_tables;

/* this callback does all the work of firing a stream of
   SDL_USEREVENTs at the owner */
Uint32 key_repeat_callback(Uint32 interval, void *owner)
{
	/* we just send a stream of userevents while a
	   key is down */

	SDL_Event e;
	e.type = SDL_USEREVENT;
	e.user.code = ST_EVENT_KEY_REPEAT;
	e.user.data1 = owner;
	e.user.data2 = NULL;
	SDL_PushEvent(&e);

	Uint32 repeat_rate = ((key_editor*) owner)->get_key_repeat_rate();

	/* regardless of what time we were setup with,
	   we want to keep firing at repeat_rate */
	return static_cast<Uint32>(repeat_rate);
}

} /* anon namespace */

/* push the current unicode state and enable unicode */
void enter_text_mode()
{
	/* query current state */
	const int was_enabled = SDL_EnableUNICODE(-1);
	unicode_stack.push(was_enabled);
	SDL_EnableUNICODE(1);
}

/* restore the previous unicode state overwritten by enter_text_mode */
void leave_text_mode()
{
	if(unicode_stack.empty()) {
		assert(false);
		return;
	}
	const int was_enabled = unicode_stack.top();
	unicode_stack.pop();
	SDL_EnableUNICODE(was_enabled);
}

std::vector<SDL_keysym> key_table::bindings(key_table::logical_key k) const
{
	std::vector<SDL_keysym> ret;

	for(const_iterator i = bindings_.begin(); i != bindings_.end(); ++i) {
		if(i->second == k) {
			ret.push_back(i->first);
		}
	}
	return ret;
}

key_table::logical_key key_table::key(const SDL_keysym& sym) const {
	const_iterator i = bindings_.find(sym);
	if(i != bindings_.end()) {
		return i->second;
	}
	return LK_NONE;
}


const key_table_ptr get_key_table(int table)
{
	std::map<int,key_table_ptr>::iterator loc = key_tables.find(table);
	if(loc == key_tables.end()) {
		key_table_ptr kt(new key_table());
		key_tables[table] = kt;
		return kt;
	}
	return loc->second;
}
void set_key_binding(int table, SDLKey key, SDLMod mod, key_table::logical_key k)
{
	SDL_keysym ks;
	ks.sym = key;
	ks.mod = mod;
	get_key_table(table)->set_binding(ks, k);
}
void clear_key_binding(int table, SDLKey key, SDLMod mod)
{
	SDL_keysym ks;
	ks.sym = key;
	ks.mod = mod;
	get_key_table(table)->clear_binding(ks);
}
void clear_key_table(int table)
{
	get_key_table(table)->clear();
}

class matches_sym_p {
public:
	matches_sym_p(const SDL_keysym& sym) : sym_(sym) {}
	bool operator()(const SDL_keysym& sym) {
		return sym_.sym == sym.sym;
	}
private:
	const SDL_keysym sym_;
};


/* setup a new key to repeat, stacking any existing repeating key */
void key_editor::start_key_repeating(const SDL_keysym& sym)
{
	if(!repeating_key_.empty()) {
		SDL_RemoveTimer(timer_id_);
	}
	timer_id_ = SDL_AddTimer(repeat_delay_, key_repeat_callback, this);
	repeating_key_.push_back(sym);
}

/* finish repeating (the most recent version of) the given key */
void key_editor::finish_key_repeating(const SDL_keysym& sym)
{
	std::vector<SDL_keysym>::reverse_iterator loc =
		std::find_if(repeating_key_.rbegin(),
			     repeating_key_.rend(),
			     matches_sym_p(sym));
	if(loc == repeating_key_.rend()) {
		/* not found */
		return;
	}
	std::vector<SDL_keysym>::iterator loc_f = loc.base() -1;

	bool was_back = loc == repeating_key_.rbegin();
	repeating_key_.erase(loc_f);
	if(was_back) {
		SDL_RemoveTimer(timer_id_);
		if(!repeating_key_.empty()) {
			timer_id_ = SDL_AddTimer(repeat_delay_, key_repeat_callback, this);
		}
	}
}

void key_editor::clear_key_repeating() {
	if(!repeating_key_.empty()) {
		SDL_RemoveTimer(timer_id_);
	}
	repeating_key_.clear();
}

bool key_editor::process_event(const SDL_Event &e, bool claimed)
{
	SDL_keysym sym;
    if(claimed) {
        return claimed;
    }

	bool sym_set = false;

	switch(e.type) {
	case SDL_KEYDOWN:
		sym = e.key.keysym;
		sym_set = true;
		start_key_repeating(sym);
		break;
	case SDL_KEYUP:
		finish_key_repeating(e.key.keysym);
		break;
	case SDL_USEREVENT:
		if(e.user.code == ST_EVENT_KEY_REPEAT && e.user.data1 == this) {
			sym = repeating_key_.back();
			sym_set = true;
			break;
		}
		break;
	}

	if(!sym_set) {
		return false;
	}
	key_table::logical_key k = kt_->key(sym);
	if(k) {
		handle_logical_key_press(k);
		return true;
	}
	if(sym.unicode) {
		/* in SDL < 1.3, sym.unicode is UCS2
		   not real unicode, but it is a compatible
		   subset that can be safely assigned to a UCS4
		   variable */
		handle_unicode_press(sym.unicode);
	}
	return false;
}

void key_editor::handle_unicode_press(Uint32 unicode)
{
	if(buf_->has_mark_set()) {
		if(buf_->mark() < buf_->caret()) {
			buf_->set_caret(buf_->erase(buf_->mark(), buf_->caret()));
		} else {
			buf_->set_caret(buf_->erase(buf_->caret(), buf_->mark()));
		}
		buf_->clear_mark();
	}
	if(!filter_->matches(unicode)) {
		buf_->insert(unicode);
	}
}

void key_editor::handle_logical_key_press(const key_table::logical_key k)
{
	iterator func = functions_.find(k);
	if(func != functions_.end()) {
		(*(func->second))(*buf_);
		return;
	}
}

bool key_set_filter::matches(Uint32 ch) {
	return chars_.find(ch) != chars_.end();
}

std::vector<Uint32> key_filter::filter(const std::vector<Uint32>& input)
{
	std::vector<Uint32> ret;

	for(std::vector<Uint32>::const_iterator i = input.begin();
	    i != input.end() ; ++i )
	{
		if(!matches(*i)) {
			ret.push_back(*i);
		}
	}
	return ret;
}

void editable_text::get_selection(int *start, int *end) const
{
	if(!mark_set_) {
		*start = -1;
		*end   = -1;
		return;
	}

	if(mark_ < caret_) {
		*start = mark_;
		*end   = caret_;
	} else {
		*start = caret_;
		*end   = mark_;
	}
}

void editable_text::store_undo_info(std::vector<edit_state>& st)
{
	struct edit_state s;
	s.text = text_;
	s.caret = caret_;
	s.mark = mark_;
	s.mark_set = mark_set_;
	st.push_back(s);
}
void editable_text::load_undo_info(std::vector<edit_state>& st)
{
	struct edit_state s = st.back();
	text_ = s.text;
	caret_ = s.caret;
	mark_ = s.mark;
	mark_set_ = s.mark_set;
	st.pop_back();
}

void editable_text::insert(Uint32 letter)
{
	store_undo_info(undo_);
	redo_.clear();

	iterator x = itor_from_int(caret_);
	if(x != text_.end()) {
		text_.insert(x, letter);
	} else {
		text_.push_back(letter);
	}
	++caret_;

}
editable_text::iterator editable_text::erase(iterator i1, iterator i2) {
	store_undo_info(undo_);
	redo_.clear();
	return text_.erase(i1, i2);
}

void editable_text::undo() {
	if(undo_.empty()) {
		return;
	}
	store_undo_info(redo_);
	load_undo_info(undo_);
}

void editable_text::redo() {
	if(redo_.empty()) {
		return;
	}
	store_undo_info(undo_);
	load_undo_info(redo_);
}


/* support for moving the caret around a buffer intelligently
   assumes no new lines */
class caret_transform {
public:
	typedef editable_text buf_type;
	virtual ~caret_transform() {}
	virtual buf_type::const_iterator operator()(const buf_type& buf,
					      const buf_type::const_iterator& caret) =0;
};

typedef boost::shared_ptr<caret_transform> caret_transform_ptr;

class pos_forward: public caret_transform {
public:
	buf_type::const_iterator operator()(const buf_type& buf,
					      const buf_type::const_iterator& caret) {
		if(caret != buf.end()) {
			return caret + 1;
		}
		return caret;
	}
};
class pos_backward: public caret_transform {
public:
	buf_type::const_iterator operator()(const buf_type& buf,
					    const buf_type::const_iterator& caret) {
		if(caret != buf.begin()) {
			return caret - 1;
		}
		return caret;
	}
};
class word_forward: public caret_transform {
public:
	buf_type::const_iterator operator()(const buf_type& buf,
					    const buf_type::const_iterator& caret) {
		return std::find_if(caret+1, buf.end(), is_breakable_p());
	}
};
class word_backward: public caret_transform {
public:
	buf_type::const_iterator operator()(const buf_type& buf,
					    const buf_type::const_iterator& caret) {
		buf_type::const_reverse_iterator caret_base = buf.rbegin();
		std::advance(caret_base, distance(caret, buf.end()));
		buf_type::const_reverse_iterator f =
			std::find_if(caret_base, buf.rend(), is_breakable_p());
		if(f != buf.rend()) {
			return f.base() -1;
		}
		return buf.begin();
	}
};
class line_forward: public caret_transform {
public:
	buf_type::const_iterator operator()(const buf_type& buf,
					    const buf_type::const_iterator& caret) {
		return buf.end();
	}
};
class line_backward: public caret_transform {
public:
	buf_type::const_iterator operator()(const buf_type& buf,
					    const buf_type::const_iterator& caret) {
		return buf.begin();
	}
};

inline void swap(editable_text::iterator& i, editable_text::iterator&j) {
	editable_text::iterator tmp;
	tmp = i;
	i = j;
	j = tmp;
}

class delete_partial: public edit_operation {
public:
	delete_partial(const caret_transform_ptr t) : t_(t) {}
	void operator()(editable_text& buf) {
		editable_text::iterator caret = buf.caret();
		editable_text::iterator mark;

		/* delete partial always deletes just the selection if there is one */
		if(buf.has_mark_set()) {
			mark = buf.mark();
		} else {
			/* otherwise figure out what to delete */
			mark = unconst(buf, (*t_)(buf, buf.caret()));
		}
		if(caret < mark) {
			swap(mark, caret);
		}

		/* at this point mark,caret enclose the section to remove */
		if(caret == mark) {
			/* empty range */
			return;
		}
		buf.set_caret(buf.erase(mark, caret));
		if(buf.has_mark_set()) {
			buf.clear_mark();
		}
	}
private:
	const caret_transform_ptr t_;
};

class delete_all: public edit_operation {
public:
	void operator()(editable_text& buf) {
		buf.clear();
	}
};

class select_partial: public edit_operation {
public:
	select_partial(const caret_transform_ptr t) : t_(t) {}
	void operator()(editable_text& buf) {
		if(!buf.has_mark_set()) {
			buf.set_mark(buf.caret());
		}
		buf.set_caret(unconst(buf, (*t_)(buf, buf.caret())));
	}
private:
	const caret_transform_ptr t_;
};

class select_all: public edit_operation {
	void operator()(editable_text& buf) {
		buf.set_mark(buf.begin());
		buf.set_caret(buf.end());
	}
};

class move_cursor: public edit_operation {
public:
	move_cursor(const caret_transform_ptr t) : t_(t) {}
	void operator()(editable_text& buf) {
		if(buf.has_mark_set()) {
			buf.clear_mark();
		}
		buf.set_caret(unconst(buf, (*t_)(buf, buf.caret())));
	}
private:
	const caret_transform_ptr t_;
};

class undo: public edit_operation {
public:
	void operator()(editable_text& buf)
	{
		buf.undo();
	}
};

class redo: public edit_operation {
	void operator()(editable_text& buf)
	{
		buf.redo();
	}
};

void key_editor::fill_function_table() {
	caret_transform_ptr one_fwd(new pos_forward());
	caret_transform_ptr one_back(new pos_backward());
	caret_transform_ptr line_fwd(new line_forward());
	caret_transform_ptr line_back(new line_backward());
	caret_transform_ptr word_fwd(new word_forward());
	caret_transform_ptr word_back(new word_backward());

	functions_[key_table::LK_CURSOR_BACKWARD] = edit_operation_ptr(new move_cursor(one_back));
	functions_[key_table::LK_CURSOR_FORWARD] = edit_operation_ptr(new move_cursor(one_fwd));
	functions_[key_table::LK_CURSOR_BACKWARD_WORD] = edit_operation_ptr(new move_cursor(word_back));
	functions_[key_table::LK_CURSOR_FORWARD_WORD] = edit_operation_ptr(new move_cursor(word_fwd));
	functions_[key_table::LK_CURSOR_BACKWARD_LINE] = edit_operation_ptr(new move_cursor(line_back));
	functions_[key_table::LK_CURSOR_FORWARD_LINE] = edit_operation_ptr(new move_cursor(line_fwd));

	functions_[key_table::LK_DELETE_BACKWARD] = edit_operation_ptr(new delete_partial(one_back));
	functions_[key_table::LK_DELETE_FORWARD] = edit_operation_ptr(new delete_partial(one_fwd));
	functions_[key_table::LK_DELETE_BACKWARD_WORD] = edit_operation_ptr(new delete_partial(word_back));
	functions_[key_table::LK_DELETE_FORWARD_WORD] = edit_operation_ptr(new delete_partial(word_fwd));
	functions_[key_table::LK_DELETE_BACKWARD_LINE] = edit_operation_ptr(new delete_partial(line_back));
	functions_[key_table::LK_DELETE_FORWARD_LINE] = edit_operation_ptr(new delete_partial(line_fwd));
	functions_[key_table::LK_DELETE_ALL] = edit_operation_ptr(new delete_all());

	functions_[key_table::LK_SELECT_BACKWARD] = edit_operation_ptr(new select_partial(one_back));
	functions_[key_table::LK_SELECT_FORWARD] = edit_operation_ptr(new select_partial(one_fwd));
	functions_[key_table::LK_SELECT_BACKWARD_WORD] = edit_operation_ptr(new select_partial(word_back));
	functions_[key_table::LK_SELECT_FORWARD_WORD] = edit_operation_ptr(new select_partial(word_fwd));
	functions_[key_table::LK_SELECT_BACKWARD_LINE] = edit_operation_ptr(new select_partial(line_back));
	functions_[key_table::LK_SELECT_FORWARD_LINE] = edit_operation_ptr(new select_partial(line_fwd));
	functions_[key_table::LK_SELECT_ALL] = edit_operation_ptr(new select_all());

	functions_[key_table::LK_UNDO] = edit_operation_ptr(new undo());
	functions_[key_table::LK_REDO] = edit_operation_ptr(new redo());
}

/* this sets up the following keys:
   left/right - move caret left/right
   home/end - caret start/end of line
   ctrl+left/right - move caret word left/right
   backspace/del - delete backward/forward
   ctrl+backspace/del - delete word backward/forward
   shift+backspace/del - delete line backward/forward
   shift + left/right - select left/right
   ctrl+shift+left/right - select word left/right
   shift + home/end - select to start/end of line
   ctrl+a - select all
   ctrl+k - delete all
   ctrl+z - undo
   ctrl+y - redo
*/
void init() {
	const int kt_num = 0;
	set_key_binding(kt_num, SDLK_LEFT, KMOD_NONE, key_table::LK_CURSOR_BACKWARD);
	set_key_binding(kt_num, SDLK_RIGHT, KMOD_NONE, key_table::LK_CURSOR_FORWARD);

	set_key_binding(kt_num, SDLK_HOME, KMOD_NONE, key_table::LK_CURSOR_BACKWARD_LINE);
	set_key_binding(kt_num, SDLK_END, KMOD_NONE, key_table::LK_CURSOR_FORWARD_LINE);

	set_key_binding(kt_num, SDLK_LEFT, KMOD_LCTRL, key_table::LK_CURSOR_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_RIGHT, KMOD_LCTRL, key_table::LK_CURSOR_FORWARD_WORD);
	set_key_binding(kt_num, SDLK_LEFT, KMOD_RCTRL, key_table::LK_CURSOR_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_RIGHT, KMOD_RCTRL, key_table::LK_CURSOR_FORWARD_WORD);

	set_key_binding(kt_num, SDLK_BACKSPACE, KMOD_NONE, key_table::LK_DELETE_BACKWARD);
	set_key_binding(kt_num, SDLK_DELETE, KMOD_NONE, key_table::LK_DELETE_FORWARD);

	/* one modifier so 2 bindings per key (left and right) */
	set_key_binding(kt_num, SDLK_BACKSPACE, KMOD_LCTRL, key_table::LK_DELETE_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_DELETE, KMOD_LCTRL, key_table::LK_DELETE_FORWARD_WORD);
	set_key_binding(kt_num, SDLK_BACKSPACE, KMOD_RCTRL, key_table::LK_DELETE_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_DELETE, KMOD_RCTRL, key_table::LK_DELETE_FORWARD_WORD);

	set_key_binding(kt_num, SDLK_BACKSPACE, KMOD_LSHIFT, key_table::LK_DELETE_BACKWARD_LINE);
	set_key_binding(kt_num, SDLK_DELETE, KMOD_LSHIFT, key_table::LK_DELETE_FORWARD_LINE);
	set_key_binding(kt_num, SDLK_BACKSPACE, KMOD_RSHIFT, key_table::LK_DELETE_BACKWARD_LINE);
	set_key_binding(kt_num, SDLK_DELETE, KMOD_RSHIFT, key_table::LK_DELETE_FORWARD_LINE);

	set_key_binding(kt_num, SDLK_k, KMOD_LCTRL, key_table::LK_DELETE_ALL);
	set_key_binding(kt_num, SDLK_k, KMOD_RCTRL, key_table::LK_DELETE_ALL);

	set_key_binding(kt_num, SDLK_LEFT, KMOD_LSHIFT, key_table::LK_SELECT_BACKWARD);
	set_key_binding(kt_num, SDLK_RIGHT, KMOD_LSHIFT, key_table::LK_SELECT_FORWARD);
	set_key_binding(kt_num, SDLK_LEFT, KMOD_RSHIFT, key_table::LK_SELECT_BACKWARD);
	set_key_binding(kt_num, SDLK_RIGHT, KMOD_RSHIFT, key_table::LK_SELECT_FORWARD);

	/* two modifiers so *4* bindings per key */
	set_key_binding(kt_num, SDLK_LEFT, static_cast<SDLMod>(KMOD_LSHIFT | KMOD_LCTRL),
			key_table::LK_SELECT_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_RIGHT, static_cast<SDLMod>(KMOD_LSHIFT | KMOD_LCTRL),
			key_table::LK_SELECT_FORWARD_WORD);
	set_key_binding(kt_num, SDLK_LEFT, static_cast<SDLMod>(KMOD_RSHIFT | KMOD_LCTRL),
			key_table::LK_SELECT_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_RIGHT, static_cast<SDLMod>(KMOD_RSHIFT | KMOD_LCTRL),
			key_table::LK_SELECT_FORWARD_WORD);
	set_key_binding(kt_num, SDLK_LEFT, static_cast<SDLMod>(KMOD_LSHIFT | KMOD_RCTRL),
			key_table::LK_SELECT_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_RIGHT, static_cast<SDLMod>(KMOD_LSHIFT | KMOD_RCTRL),
			key_table::LK_SELECT_FORWARD_WORD);
	set_key_binding(kt_num, SDLK_LEFT, static_cast<SDLMod>(KMOD_RSHIFT | KMOD_RCTRL),
			key_table::LK_SELECT_BACKWARD_WORD);
	set_key_binding(kt_num, SDLK_RIGHT, static_cast<SDLMod>(KMOD_RSHIFT | KMOD_RCTRL),
			key_table::LK_SELECT_FORWARD_WORD);

	set_key_binding(kt_num, SDLK_END, KMOD_LSHIFT, key_table::LK_SELECT_FORWARD_LINE);
	set_key_binding(kt_num, SDLK_END, KMOD_RSHIFT, key_table::LK_SELECT_FORWARD_LINE);
	set_key_binding(kt_num, SDLK_HOME, KMOD_LSHIFT, key_table::LK_SELECT_BACKWARD_LINE);
	set_key_binding(kt_num, SDLK_HOME, KMOD_RSHIFT, key_table::LK_SELECT_BACKWARD_LINE);

	set_key_binding(kt_num, SDLK_a, KMOD_LCTRL, key_table::LK_SELECT_ALL);
	set_key_binding(kt_num, SDLK_a, KMOD_RCTRL, key_table::LK_SELECT_ALL);

	set_key_binding(kt_num, SDLK_z, KMOD_LCTRL, key_table::LK_UNDO);
	set_key_binding(kt_num, SDLK_z, KMOD_RCTRL, key_table::LK_UNDO);
	set_key_binding(kt_num, SDLK_y, KMOD_LCTRL, key_table::LK_REDO);
	set_key_binding(kt_num, SDLK_y, KMOD_RCTRL, key_table::LK_REDO);
}
} /* namespace text */

bool text_box::handle_event(const SDL_Event &e, bool claimed)
{
	/* first our own processing for input focus */
    if(claimed) {
        return claimed;
    }

	bool hidden_from_editors = false;
	switch(e.type) {
	case SDL_MOUSEBUTTONDOWN:
		if(hit_me(e) && state() != DISABLED) {
			set_state(SELECTED);
			claimed = true;
		} else {
			set_state(NORMAL);
		}
		break;
	case SDL_MOUSEMOTION:
		if(state() == NORMAL && hit_me(e)) {
			set_state(HIGHLIGHTED);
		} else if(state() == HIGHLIGHTED && !hit_me(e)) {
			set_state(NORMAL);
		}
		break;
	case SDL_KEYDOWN:
		switch(e.key.keysym.sym) {
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
		case SDLK_ESCAPE:
			/* mask these events which might be significant
			   to someone else */
			hidden_from_editors = true;
			if(state() == SELECTED) {
				set_state(NORMAL);
			}
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	bool edit_claimed = false;
	if(state() == SELECTED && !hidden_from_editors) {
		foreach(text::editor_ptr edit, editors_) {
			edit_claimed = edit->process_event(e, claimed);
			if(edit_claimed) break;
		}
	}
	return edit_claimed || claimed;
}

void text_box::inner_draw() const
{
	if(edit_->text() != cached_text_  ||
	   cached_state_ != state() ||
	   state()==SELECTED) {
		recalculate_texture();
	}
	int pos_x = 0;
	if(state() == SELECTED) {
		const int tw = texture_.width();
		if(tw > width()) {
			/* place the caret as close to the center of area as possible */
			int w,h;
			graphics::font::get_text_size(cached_text_.substr(0, edit_->caret_index()),
						      font_size_, &w, &h);
			pos_x = width()/2 - w;
			if(pos_x > 0) { /* would underrun to the left */
				pos_x = 0;
			} else if(tw + pos_x < width()) {  /* would underrun to the right */
				pos_x = width() - tw;
			}
		}
	}
	SDL_Rect r = { x(), y(), width(), height() };
	graphics::push_clip(r);
	graphics::blit_texture(texture_, x() + pos_x, y());
	graphics::pop_clip();
}

void text_box::recalculate_texture() const
{
	cached_text_ = edit_->text();
	cached_state_ = static_cast<STATE>(state());

	int sel_start = -1;
	int sel_end = -1;
	int caret = -1;

	if(cached_state_ == SELECTED) {
		caret = edit_->caret_index();
	}
	if(edit_->has_selection()) {
		edit_->get_selection(&sel_start, &sel_end);
	}

	texture_ = graphics::font::render_complex_text(cached_text_, font_size_, color_,
						       caret_fg_, caret_bg_, opaque_caret_,
						       selection_fg_, selection_bg_, opaque_selection_,
						       caret, sel_start, sel_end);

}

} /* namespace gui */

