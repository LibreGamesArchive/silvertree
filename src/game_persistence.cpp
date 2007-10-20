#include "dialog.hpp"
#include "filesystem.hpp"
#include "foreach.hpp"
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

class persistence_dialog: public gui::dialog {
public:
	friend class persistence_ok_button;
	friend class persistence_cancel_button;

	persistence_dialog(game_logic::world *wp, const std::string& dir)
		: dialog(0,0,graphics::screen_width(), graphics::screen_height()),
		  wp_(wp), dir_(dir), was_cancelled_(false)
	{
		set_clear_bg(false);
	}
	virtual ~persistence_dialog() {}
	std::string selected_file() { return dir_ + "/" + do_selected_file(); }
	virtual bool has_selected_file() { return !was_cancelled_; }
protected:
	void handle_draw() const;
	bool handle_event(const SDL_Event &e);
	virtual std::string do_selected_file() =0;
private:
	game_logic::world *wp_;
	const std::string dir_;
	bool was_cancelled_;
};

class persistence_ok_button: public gui::labelled_button_widget {
public:
	persistence_ok_button(persistence_dialog* dlg,
			   const std::string& skin_prefix)
		: dialog_(dlg)
	{

		add_skin(skin_prefix + "-ok-button-skin-normal", gui::button_widget::NORMAL);
		add_skin(skin_prefix + "-ok-button-skin-highlighted", gui::button_widget::HIGHLIGHTED);
		add_skin(skin_prefix + "-ok-button-skin-clicked", gui::button_widget::CLICKED);
		set_label_text("Ok");
		set_hotkey(SDLK_RETURN, KMOD_NONE);
	}
	void clicked() {
		dialog_->was_cancelled_ = false;
		dialog_->close();
	}
private:
	persistence_dialog* dialog_;
};

class persistence_cancel_button: public gui::labelled_button_widget {
public:
	persistence_cancel_button(persistence_dialog* dlg,
				  const std::string& skin_prefix)
		: dialog_(dlg)
	{
		add_skin(skin_prefix + "-cancel-button-skin-normal", gui::button_widget::NORMAL);
		add_skin(skin_prefix + "-cancel-button-skin-highlighted", gui::button_widget::HIGHLIGHTED);
		add_skin(skin_prefix + "-cancel-button-skin-clicked", gui::button_widget::CLICKED);
		set_label_text("Cancel");
		set_hotkey(SDLK_ESCAPE, KMOD_NONE);
	}
	void clicked() {
		dialog_->was_cancelled_ = true;
		dialog_->close();
	}
private:
	persistence_dialog* dialog_;
};

class save_dialog: public persistence_dialog {
public:
	save_dialog(game_logic::world *wp,
		    const std::string& dir=sys::get_saves_dir(),
		    const std::string& txt = "")
		: persistence_dialog(wp, dir)
	{
		construct_interface(dir, txt);
	}
protected:
	std::string do_selected_file() { return text_box_->text(); }
private:
	void construct_interface(const std::string& dir, const std::string& txt);
	gui::text_box_ptr text_box_;
};

class load_dialog_item;

class load_dialog: public persistence_dialog {
public:
	friend class load_dialog_item;
	load_dialog(game_logic::world *wp,
		    const std::string& dir = sys::get_saves_dir(),
		    const std::string& txt = "")
		: persistence_dialog(wp, dir)
	{
		construct_interface(dir, txt);
	}
	bool has_selected_file()
	{
		return persistence_dialog::has_selected_file() && !selected_.empty();
	}
protected:
	std::string do_selected_file() { return selected_; }
private:
	void construct_interface(const std::string& dir, const std::string& txt);

	std::string selected_;
	gui::scrolled_container_ptr scrolly_;
};

class load_dialog_item : public gui::labelled_button_widget {
public:
	load_dialog_item(load_dialog *dlg) : dlg_(dlg) {}
	void clicked()
	{
		dlg_->selected_ = label_text();
	}
private:
	load_dialog *dlg_;
};

void persistence_dialog::handle_draw() const
{
	wp_->draw();
	handle_draw_children();
}

bool persistence_dialog::handle_event(const SDL_Event& e)
{
	return handle_event_children(e);
}

void save_dialog::construct_interface(const std::string& dir, const std::string& txt)
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
	gui::widget_ptr confirm_button(new persistence_ok_button(this, "save-dialog"));
	confirm_button = gui::frame_manager::make_frame(confirm_button, "save-dialog-ok-button-frame");
	inner->add_widget(confirm_button, dialog::MOVE_RIGHT);
	gui::widget_ptr cancel_button(new persistence_cancel_button(this, "save-dialog"));
	cancel_button = gui::frame_manager::make_frame(cancel_button, "save-dialog-cancel-button-frame");
	inner->add_widget(cancel_button, dialog::MOVE_RIGHT);

	set_cursor(graphics::screen_width()/4, graphics::screen_height()/4);
	add_widget(inner);
}

void load_dialog::construct_interface(const std::string& dir, const std::string& filename)
{

	gui::framed_dialog_ptr inner(new gui::framed_dialog(0,0, graphics::screen_width()/2,
							    graphics::screen_height()/2));
	inner->set_frame(gui::frame_manager::make_frame(inner, "load-dialog-frame"));
	inner->set_padding(0);

	gui::widget_ptr title(new gui::label("Load game", graphics::color_white()));
	title = gui::frame_manager::make_frame(title, "load-dialog-title-frame");
	inner->set_cursor((inner->width()-title->width())/2, 0);
	inner->add_widget(title, dialog::MOVE_DOWN);

	inner->set_cursor(0, inner->cursor_y());

	std::vector<std::string> files;
	sys::get_files_in_dir(dir, &files);
	std::cout << "Reading dir "<<dir<<"\n";
	std::cout << "found "<<files.size()<<" files\n";

	scrolly_ = gui::scrolled_container_ptr(new gui::scrolled_container(gui::scrolled_container::VERTICAL));
	foreach(std::string filename, files) {
		gui::labelled_button_widget_ptr item(new load_dialog_item(this));
		item->set_label_text(filename);
		item->add_skin("load-dialog-item-skin-normal", gui::button_widget::NORMAL);
		item->add_skin("load-dialog-item-skin-highlighted", gui::button_widget::HIGHLIGHTED);
		item->add_skin("load-dialog-item-skin-clicked", gui::button_widget::CLICKED);
		std::cout << "added file "<<filename<<" to the listing\n";
		scrolly_->add_widget(item);
	}
	gui::frame_ptr box = gui::frame_manager::make_frame(scrolly_, "load-dialog-scroll-frame");

	gui::widget_ptr confirm_button(new persistence_ok_button(this, "load-dialog"));
	confirm_button = gui::frame_manager::make_frame(confirm_button, "load-dialog-ok-button-frame");
	gui::widget_ptr cancel_button(new persistence_cancel_button(this, "load-dialog"));
	cancel_button = gui::frame_manager::make_frame(cancel_button, "load-dialog-cancel-button-frame");

	const int used_height = std::max(confirm_button->height(), cancel_button->height()) + title->height();

	box->set_dim(inner->width(), inner->height() - used_height);

	inner->add_widget(box, dialog::MOVE_RIGHT);
	inner->add_widget(gui::widget_ptr(new gui::scroll_button(scrolly_, -1)), dialog::MOVE_DOWN);
	inner->add_widget(gui::widget_ptr(new gui::scroll_button(scrolly_, 1)), dialog::MOVE_DOWN);
	inner->set_cursor(0, box->y() + box->height());
	inner->add_widget(confirm_button, dialog::MOVE_RIGHT);
	inner->add_widget(cancel_button, dialog::MOVE_RIGHT);

	set_cursor(graphics::screen_width()/4, graphics::screen_height()/4);
	add_widget(inner);
}


void do_save(const std::string& filename)
{
	std::string data;
	assert(!game_logic::world::current_world_stack().empty());
	wml::node_ptr node(new wml::node("game"));
	node->add_child(game_logic::world::current_world_stack().front()->write());
	game_logic::global_game_state::get().write(node);
	wml::write(node, data);
	sys::write_file(filename, data);
}

void silent_save(const std::string& filename)
{
	do_save(filename);
}

bool save(const std::string& start_filename, game_logic::world *wp)
{
	// save the game
	save_dialog s(wp, sys::get_saves_dir(), start_filename);
	s.show_modal();

	if(!s.has_selected_file())
	{
		return false;
	}
	do_save(s.selected_file());
	return true;
}

void do_load(const std::string& filename)
{
	throw game_logic::world::new_game_exception(filename);
}


void silent_load(const std::string& filename)
{
	do_load(filename);
}

bool load(const std::string& start_filename, game_logic::world *wp)
{
	std::cout << "Loading game\n";
	load_dialog l(wp, sys::get_saves_dir(), start_filename);
	l.show_modal();

	if(!l.has_selected_file() || !sys::file_exists(l.selected_file()))
	{
		std::cout << "No file selected\n";
		return false;
	}
	std::cout << "Selected file "<<l.selected_file()<<"\n";
	do_load(l.selected_file());
	// we will never get here since a
	// new_game_exception will be thrown
	return true;
}

} /* namespace game_dialogs */
