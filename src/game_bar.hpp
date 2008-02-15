#ifndef GAME_BAR_HPP_INCLUDED
#define GAME_BAR_HPP_INCLUDED

#include <vector>

#include "character.hpp"
#include "dialog.hpp"
#include "frame.hpp"
#include "gui_core.hpp"
#include "label.hpp"
#include "party.hpp"

namespace game_dialogs {

class game_bar : public gui::framed_dialog {
public:
	game_bar(int x, int y, int w, int h, game_logic::party_ptr pty,
		 game_logic::world *wp)
		: gui::framed_dialog(x,y,w,h) {
		construct_interface(pty, wp);
	}

	SDL_Rect character_rect(int index) const;

private:
	bool handle_event(const SDL_Event &e, bool claimed);
	void construct_interface(game_logic::party_ptr pty, game_logic::world *wp);
	void inner_draw() const;

	gui::const_widget_ptr portrait_set_;
	std::vector<gui::const_widget_ptr> char_rects_;
};

typedef boost::shared_ptr<game_bar> game_bar_ptr;

class game_bar_party_button : public gui::button_widget {
public:
	game_bar_party_button(game_logic::party_ptr pty);
private:
	void inner_draw() const;
	void clicked();

	gui::label_ptr label_;
	game_logic::party_ptr pty_;
	mutable gui::frame_ptr frame_;
	mutable int old_cash_;
};

class game_bar_portrait_set : public gui::dialog {
public:
	game_bar_portrait_set(game_logic::party_ptr pty, int w, int h,
	                      std::vector<gui::const_widget_ptr>* char_rects)
		: dialog(0,0,w,h), pty_(pty), char_rects_(char_rects)
	{
		construct_interface();
	}
private:
	void handle_draw() const;
	void construct_interface();
	void build_scrolly() const;
	game_logic::party_ptr pty_;
	mutable gui::scrolled_container_ptr scrolly_;
	mutable std::vector<gui::const_widget_ptr>* char_rects_;
};

class game_bar_portrait_button : public gui::rollin_menu_widget {
public:
	game_bar_portrait_button(game_logic::character_ptr ch, game_logic::party_ptr pty);
private:
	void option_selected(int i);
	void inner_draw() const;
	void construct_interface();
	mutable SDL_Rect p_dims_;

	gui::widget_ptr portrait_;
	game_logic::character_ptr ch_;
	game_logic::party_ptr pty_;
};

class game_bar_game_button : public gui::popup_menu_widget {
public:
	game_bar_game_button(game_logic::world *wp);
	// more functionality later
private:
	void option_selected(int opt);

	game_logic::world *wp_;
};

}

#endif
