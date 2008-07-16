#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <sstream>
#include <vector>

#include "character_equip_dialog.hpp"
#include "character_status_dialog.hpp"
#include "game_bar.hpp"
#include "game_persistence.hpp"
#include "image_widget.hpp"
#include "learn_skills_dialog.hpp"
#include "party_status_dialog.hpp"
#include "raster.hpp"
#include "text.hpp"
#include "widget.hpp"
#include "world.hpp"

namespace game_dialogs {

SDL_Rect game_bar::character_rect(int index) const
{
    /* FIXME: this surely fails if the bar is scrolled */
	assert(index >= 0);
	if(index >= char_rects_.size()) {
		SDL_Rect res = {0, 0, 0, 0};
		return res;
	}
	assert(portrait_set_);
	gui::const_widget_ptr w = char_rects_[index];
	SDL_Rect res = { x() + portrait_set_->x(), y(), w->width(), w->height() };
	for(int n = 0; n < index; ++n) {
		res.x += char_rects_[index]->width();
	}
	return res;
}

void game_bar::inner_draw() const
{
	handle_draw_children();
}

bool game_bar::handle_event(const SDL_Event &e, bool claimed)
{
    claimed = handle_event_children(e, claimed);

    if(!claimed && hit_me(e)) {
        claimed = true;
    }

    return claimed;
}

void game_bar::construct_interface(game_logic::party_ptr pty, game_logic::world *wp)
{
	set_padding(0);

	// add a game_bar_button for the party status widget
	gui::button_widget_ptr party_button(new game_bar_party_button(pty));
	party_button->add_skin("game-bar-party-button-skin-normal", gui::button_widget::NORMAL);
	party_button->add_skin("game-bar-party-button-skin-highlighted", gui::button_widget::HIGHLIGHTED);
	party_button->add_skin("game-bar-party-button-skin-clicked", gui::button_widget::CLICKED);
	party_button->set_dim(party_button->width(), height());
	party_button->set_hotkey(SDLK_s, KMOD_NONE);
	add_widget(party_button, dialog::MOVE_RIGHT);

	// make  silvertree game_bar_button for the game menu, but add it later
	gui::menu_widget_ptr game_button(new game_bar_game_button(wp));
	game_button->set_dim(game_button->width(), height());
	game_button->set_hotkey(SDLK_ESCAPE, KMOD_NONE);

	{
		char_rects_.clear();
		// add a scrollable char portrait widgets
		gui::widget_ptr w(new game_bar_portrait_set(pty, width() - party_button->width()- game_button->width(), 
                                                    height(), &char_rects_));
		add_widget(w, dialog::MOVE_RIGHT);
		portrait_set_ = w;
	}

	add_widget(game_button, dialog::MOVE_RIGHT);
}

game_bar_party_button::game_bar_party_button(game_logic::party_ptr pty)
	: pty_(pty), old_cash_(-1)
{
	label_.reset(new gui::label("", graphics::color_white(), 14));
}

void game_bar_party_button::inner_draw() const
{
	const int cash = pty_->money();
	if(cash != old_cash_) {
		old_cash_ = cash;

		std::stringstream str;
		str << cash;
		label_->set_text(str.str());
		frame_ = gui::frame_manager::make_frame(label_, "game-bar-party-button-text");
		frame_->set_loc(x(),y());
	}
	frame_->draw();
}

void game_bar_party_button::clicked()
{
	party_status_dialog(pty_).show_modal();
}

void game_bar_portrait_set::build_scrolly() const
{
	int count = 0;
	scrolly_->clear();
	for(std::vector<game_logic::character_ptr>::const_iterator i = pty_->members().begin();
	    i != pty_->members().end(); ++i)
	{
		gui::widget_ptr w(new game_bar_portrait_button(*i, pty_));
		w = gui::frame_manager::make_frame(w, "game-bar-portrait-frame");
		w->set_dim(w->width(), height());
		scrolly_->add_widget(w);
		if(char_rects_) {
			char_rects_->push_back(w);
		}
		++count;
	}
}

void game_bar_portrait_set::construct_interface()
{
	set_padding(0);

	scrolly_.reset(new gui::scrolled_container());
	build_scrolly();

	gui::button_widget_ptr left_button(new gui::scroll_button(scrolly_, -1));
	gui::button_widget_ptr right_button(new gui::scroll_button(scrolly_, 1));

	left_button->add_skin("game-bar-char-scroll-left-skin-normal", gui::button_widget::NORMAL);
	left_button->add_skin("game-bar-char-scroll-left-skin-highlighted", gui::button_widget::HIGHLIGHTED);
	left_button->add_skin("game-bar-char-scroll-left-skin-clicked", gui::button_widget::CLICKED);
	left_button->add_skin("game-bar-char-scroll-left-skin-disabled", gui::button_widget::DISABLED);
	right_button->add_skin("game-bar-char-scroll-right-skin-normal", gui::button_widget::NORMAL);
	right_button->add_skin("game-bar-char-scroll-right-skin-highlighted", gui::button_widget::HIGHLIGHTED);
	right_button->add_skin("game-bar-char-scroll-right-skin-clicked", gui::button_widget::CLICKED);
	right_button->add_skin("game-bar-char-scroll-right-skin-disabled", gui::button_widget::DISABLED);

	gui::frame_ptr left_button_frame = gui::frame_manager::make_frame(left_button, "game-bar-char-scroll-left-frame");
	gui::frame_ptr right_button_frame = gui::frame_manager::make_frame(right_button, "game-bar-char-scroll-right-frame");

	left_button_frame->set_dim(left_button_frame->width(), height()/2);
	right_button_frame->set_dim(right_button_frame->width(), height()/2);

	gui::widget_ptr scroll_frame = gui::frame_manager::make_frame(scrolly_, "game-bar-char-scrollpane-frame");

	int lw = left_button->width();
	int rw = right_button->width();
	int w = lw > rw ? lw : rw;

	scroll_frame->set_dim(width() - w, height());
	add_widget(scroll_frame, gui::dialog::MOVE_RIGHT);
	add_widget(right_button_frame, gui::dialog::MOVE_DOWN);
	add_widget(left_button_frame, gui::dialog::MOVE_DOWN);
}

void game_bar_portrait_set::handle_draw() const {
	if(pty_->members().size()-1 != scrolly_->max_offset()) {
		build_scrolly();
	}
	handle_draw_children();
}

const SDL_Color text_color = { 0xE9, 0xD6, 0x7E };

game_bar_portrait_button::game_bar_portrait_button(game_logic::character_ptr ch, game_logic::party_ptr pty)
	: rollin_menu_widget("game-bar-portrait-option-text-frame", text_color, 16,
			     "game-bar-portrait-option-frame", "game-bar-portrait-menu-frame"),
	  ch_(ch), pty_(pty)
{
	add_option("Skills", 0);
	add_option("Statistics", 1);
	add_option("Equipment", 2);

	construct_interface();
}

void game_bar_portrait_button::option_selected(int i) {
	switch(i) {
	case 0:
		learn_skills_dialog(ch_).show_modal();
		break;
	case 1:
		character_status_dialog(ch_, pty_).show_modal();
		break;
	case 2:
		character_equip_dialog(ch_, pty_).show_modal();
		break;
	default:
		break;
	}
}

void game_bar_portrait_button::construct_interface() {
	std::string portrait_file = ch_->portrait(true);
	if(!portrait_file.empty()) {
		portrait_.reset(new gui::image_widget(portrait_file));
	}
	p_dims_.x = x();
	p_dims_.y = y();
	p_dims_.w = width();
	p_dims_.h = height();

	add_skin("game-bar-portrait-skin-normal", gui::rollin_menu_widget::NORMAL);
	add_skin("game-bar-portrait-skin-rolledover", gui::rollin_menu_widget::ROLLED_OVER);

	add_option_skin("game-bar-portrait-menu-skin-normal", gui::menu_option::NORMAL);
	add_option_skin("game-bar-portrait-menu-skin-highlighted", gui::menu_option::HIGHLIGHTED);
	add_option_skin("game-bar-portrait-menu-skin-clicked", gui::menu_option::CLICKED);
	add_option_skin("game-bar-portrait-menu-skin-disabled", gui::menu_option::DISABLED);
}

namespace {

void draw_bar(int x, int y, int w, int h, int level, int max_level,
	      const SDL_Color& level_bar_color, const SDL_Color& max_bar_color,
	      const SDL_Color& level_text_color, const SDL_Color& max_text_color)
{
	{
		SDL_Rect r = { x, y, w, h };
		graphics::draw_rect(r, max_bar_color, 255);
	}

	int bar_max_level = max_level;
	if(bar_max_level < 0) {
		bar_max_level = 0;
	}

	int bar_level = level;
	if(bar_level < 0) {
		bar_level = 0;
	} else if(bar_level > bar_max_level) {
		bar_level = bar_max_level;
	}

	int level_px = bar_max_level == 0 ? w : w*bar_level/bar_max_level;
	{
		SDL_Rect r = { x, y, level_px, h };
		graphics::draw_rect(r, level_bar_color, 255);
	}
        text::renderer& renderer = text::renderer::instance();
	{
            std::stringstream s;
            s << max_level;
            text::rendered_text_ptr t = renderer.render(s.str(), 12, max_text_color);
            t->blit(x + w - t->width() - 4, y + (h - t->height())/2);
	}
	{
            std::stringstream s;
            s << level;
            text::rendered_text_ptr t = renderer.render(s.str(), 12, level_text_color);
            t->blit(x + 4, y + (h - t->height())/2);
	}
}
}

void game_bar_portrait_button::inner_draw() const
{
	if(portrait_) {
		if(p_dims_.x != x() || p_dims_.y != y() || p_dims_.w != width() || p_dims_.h != height()) {
			GLfloat portrait_aspect = portrait_->width()/static_cast<GLfloat>(portrait_->height());
			GLfloat my_aspect = width()/static_cast<GLfloat>(height());
			if(my_aspect == portrait_aspect) {
				portrait_->set_loc(x(), y());
				portrait_->set_dim(width(), height());
			} else if(portrait_aspect > my_aspect) {
				// too wide
				int pw = static_cast<int>(height()*portrait_aspect);
				portrait_->set_dim(pw, height());
				portrait_->set_loc(x() - (pw-width())/2, y());
			} else {
				// too narrow
				int ph = static_cast<int>(width()/portrait_aspect);
				portrait_->set_dim(width(), ph);
				portrait_->set_loc(x(), y() - (ph-height())/2);
			}
			p_dims_.x = x();
			p_dims_.y = y();
			p_dims_.w = width();
			p_dims_.h = height();
		}
		graphics::push_clip(p_dims_);
		portrait_->draw();
		graphics::pop_clip();
	}
	SDL_Color fatigue_present = { 42, 49, 65 };
	SDL_Color fatigue_missing = { 26, 111, 202 };
	SDL_Color health_present = { 4, 40, 12 };
	SDL_Color health_missing = { 221, 2, 0 };

	draw_bar(x() + width()/24, y() + height()*6/8 - 1, width()-width()/12, height()/8,
		 ch_->hitpoints(), ch_->max_hitpoints(),
		 health_present, health_missing, graphics::color_white(), graphics::color_white());

	draw_bar(x() + width()/24, y() + height()*7/8 - 1, width()-width()/12, height()/8,
		 ch_->fatigue()/10, ch_->stamina()/10,
		 fatigue_missing, fatigue_present, graphics::color_white(), graphics::color_white());

	rollin_menu_widget::inner_draw();
}

game_bar_game_button::game_bar_game_button(game_logic::world *wp)
	: popup_menu_widget("game-bar-game-menu-option-text-frame", graphics::color_white(), 16,
			    "game-bar-game-menu-option-frame",
			    "game-bar-game-menu-frame"),
	  wp_(wp)
{
	add_skin("game-bar-game-button-skin-normal", gui::popup_menu_widget::NORMAL);
	add_skin("game-bar-game-button-skin-highlighted", gui::popup_menu_widget::HIGHLIGHTED);
	add_skin("game-bar-game-button-skin-depressed", gui::popup_menu_widget::DEPRESSED);
	add_skin("game-bar-game-button-skin-depressed-highlighted", gui::popup_menu_widget::DEPRESSED_HIGHLIGHTED);
	add_skin("game-bar-game-button-skin-clicked", gui::popup_menu_widget::CLICKED);

	add_option_skin("game-bar-game-menu-skin-normal", gui::menu_option::NORMAL);
	add_option_skin("game-bar-game-menu-skin-highlighted", gui::menu_option::HIGHLIGHTED);
	add_option_skin("game-bar-game-menu-skin-clicked", gui::menu_option::CLICKED);
	add_option_skin("game-bar-game-menu-skin-disabled", gui::menu_option::DISABLED);

	add_option("New game", 0);
	add_option("Load game", 1);
	add_option("Save game", 2);
	add_option("Credits", 3);
	add_option("Quit", 4);
	set_option_enabled(3, false);
}

void game_bar_game_button::option_selected(int opt) {
	switch(opt) {
	case 0:
		throw game_logic::world::new_game_exception("data/scenario.cfg");
		break;
	case 1:
		game_dialogs::load("silvertree-save", wp_);
		break;
	case 2: {
		game_dialogs::save("silvertree-save", wp_);
		break;
	}
	case 4: {
		SDL_Event e;
		e.type = SDL_QUIT;
		SDL_PushEvent(&e);
		break;
	}
	default:
		assert(false);
	}
}

}
