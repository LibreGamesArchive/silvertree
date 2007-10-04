#include "dialog.hpp"
#include "filesystem.hpp"
#include "frame.hpp"
#include "global_game_state.hpp"
#include "gui_core.hpp"
#include "label.hpp"
#include "preferences.hpp"
#include "text_gui.hpp"
#include "widget.hpp"
#include "wml_parser.hpp"
#include "wml_writer.hpp"
#include "world.hpp"

namespace game_dialogs {

/* forward decls */
class save_button;
class save_cancel_button;

class save_dialog: public gui::dialog {
public:
	friend class save_button;
	friend class save_cancel_button;

	explicit save_dialog(game_logic::world *wp, const std::string& txt = "") 
		: dialog(0,0,graphics::screen_width(), graphics::screen_height()),
		  wp_(wp), was_cancelled_(false)
	{
		set_clear_bg(false);
		construct_interface(txt);
	}
	std::string selected_file() { return text_box_->text(); }
	bool has_selected_file() { return !was_cancelled_; }
protected:
	void handle_draw() const;
	bool handle_event(const SDL_Event &e);
private:
	void construct_interface(const std::string& txt);
	
	game_logic::world *wp_;
	bool was_cancelled_;
	gui::text_box_ptr text_box_;
};

class save_button: public gui::labelled_button_widget {
public:
	save_button(save_dialog& dlg) : dialog_(dlg) 
	{
		add_skin("save-dialog-ok-button-skin-normal", gui::button_widget::NORMAL);
		add_skin("save-dialog-ok-button-skin-highlighted", gui::button_widget::HIGHLIGHTED);
		add_skin("save-dialog-ok-button-skin-clicked", gui::button_widget::CLICKED);
		set_label_text("Ok");
		set_hotkey(SDLK_RETURN, KMOD_NONE);
	}
	void clicked() {
		dialog_.was_cancelled_ = false;
		dialog_.close();
	}
private:
	save_dialog& dialog_;
};

class save_cancel_button: public gui::labelled_button_widget {
public:	
	save_cancel_button(save_dialog& dlg) : dialog_(dlg) 
	{
		add_skin("save-dialog-cancel-button-skin-normal", gui::button_widget::NORMAL);
		add_skin("save-dialog-cancel-button-skin-highlighted", gui::button_widget::HIGHLIGHTED);
		add_skin("save-dialog-cancel-button-skin-clicked", gui::button_widget::CLICKED);
		set_label_text("Cancel");
		set_hotkey(SDLK_ESCAPE, KMOD_NONE);
	}
	void clicked() {
		dialog_.was_cancelled_ = true;
		dialog_.close();
	}
private:
	save_dialog& dialog_;
};

void save_dialog::construct_interface(const std::string& txt)
{
	gui::text::key_set_filter_ptr filter(new gui::text::key_set_filter());
	/* prevent the most unhelpful characters from being used in a save file name */
	filter->add_key('\t');
	filter->add_key('\v');
	filter->add_key('\b');
	filter->add_key('\r');
	filter->add_key('/');
	filter->add_key('\\');
	filter->add_key(':');

	gui::framed_dialog_ptr inner(new gui::framed_dialog(0,0,
							    graphics::screen_width()/2,
							    graphics::screen_height()/2));
	inner->set_frame(gui::frame_manager::make_frame(inner, "save-dialog-frame"));
	inner->set_padding(0);

	gui::widget_ptr title(new gui::label("Save your game", graphics::color_white()));
	title = gui::frame_manager::make_frame(title, "save-dialog-title-frame");
	inner->set_cursor((inner->width()-title->width())/2, 0);
	inner->add_widget(title, dialog::MOVE_DOWN);

	inner->set_cursor(0, inner->cursor_y());
	text_box_ = gui::text_box_ptr(new gui::text_box(txt, filter));
	text_box_->add_skin("save-dialog-text-skin-normal", gui::text_box::NORMAL);
	text_box_->add_skin("save-dialog-text-skin-highlighted", gui::text_box::HIGHLIGHTED);
	text_box_->add_skin("save-dialog-text-skin-selected", gui::text_box::SELECTED);
	gui::frame_ptr box = gui::frame_manager::make_frame(text_box_, "save-dialog-text-frame");
	inner->add_widget(box, dialog::MOVE_DOWN);
	gui::widget_ptr confirm_button(new save_button(*this));
	confirm_button = gui::frame_manager::make_frame(confirm_button, "save-dialog-ok-button-frame");
	inner->add_widget(confirm_button, dialog::MOVE_RIGHT);
	gui::widget_ptr cancel_button(new save_cancel_button(*this));
	cancel_button = gui::frame_manager::make_frame(cancel_button, "save-dialog-cancel-button-frame");
	inner->add_widget(cancel_button, dialog::MOVE_RIGHT);

	set_cursor(graphics::screen_width()/4, graphics::screen_height()/4);
	add_widget(inner);
}

void save_dialog::handle_draw() const {
	wp_->draw();
	handle_draw_children();
}

bool save_dialog::handle_event(const SDL_Event& e) 
{
	return handle_event_children(e);
}

void do_save(const std::string& filename) {
	std::string data;
	assert(!game_logic::world::current_world_stack().empty());
	wml::node_ptr node(new wml::node("game"));
	node->add_child(game_logic::world::current_world_stack().front()->write());
	game_logic::global_game_state::get().write(node);
	wml::write(node, data);
	sys::write_file(filename, data);
}

void silent_save(const std::string& filename) {
	do_save(filename);
}

bool save(const std::string& start_filename, game_logic::world *wp) {
	// save the game
	std::cout << "Saving game\n";
	save_dialog s(wp, start_filename);
	s.show_modal();

	if(!s.has_selected_file()) {
		return false;
	}
	do_save(s.selected_file());
	return true;
}

} /* namespace game_dialogs */
