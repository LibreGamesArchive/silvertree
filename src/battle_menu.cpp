#include "battle_menu.hpp"
#include "battle_move.hpp"
#include "character.hpp"
#include "foreach.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "label.hpp"
#include "raster.hpp"

namespace gui {

namespace {
	const int category_size = 120;
	const int small_icon_size = 50;
	const int large_icon_size = 100;
}

battle_menu::battle_menu(const game_logic::battle& b,
                         const game_logic::battle_character& c)
	: battle_(b), char_(c), selected_(-1), selection_made_(false)
{
	const std::vector<game_logic::const_battle_move_ptr>& moves = char_.get_character().battle_moves();
	assert(!moves.empty());
	bool found_move = false;
	foreach(game_logic::const_battle_move_ptr move, moves) {
		if(!b.can_make_move(c, *move)) {
			continue;
		}

		found_move = true;

		submenu* m = NULL;
		foreach(submenu& mn, menu_) {
			if(mn.category == move->category()) {
				m = &mn;
				break;
			}
		}

		if(m == NULL) {
			menu_.push_back(submenu());
			m = &menu_.back();
			m->tex = graphics::texture::get(move->category() + "-icon.png");
			m->category = move->category();
		}

		m->options.push_back(move);
	}

	assert(found_move);

	set_dim(category_size*menu_.size(),category_size);
	set_loc((graphics::screen_width() - width())/2,
	        graphics::screen_height() - category_size*2);

	select(0);
}

game_logic::const_battle_move_ptr battle_menu::selected_move() const
{
	if(!selection_made_ || !grid_) {
		return game_logic::const_battle_move_ptr();
	}

	assert(selected_ >= 0 && selected_ < menu_.size());
	assert(grid_->selection() < menu_[selected_].options.size());
	return menu_[selected_].options[grid_->selection()];
}

void battle_menu::handle_draw() const
{
	for(int n = 0; n != menu_.size(); ++n) {
		const int size = (selected_ == n) ? large_icon_size : small_icon_size;
		const int xpos = x() + n*category_size + category_size/2 - size/2;
		const int ypos = y() + category_size/2 - size/2;
		graphics::blit_texture(menu_[n].tex, xpos, ypos, size, size);
	}

	if(grid_) {
		grid_->draw();
	}
}

bool battle_menu::handle_event(const SDL_Event& event)
{
	bool claimed = false;

	if(grid_) {
		claimed = grid_->process_event(event);
	}

	if(claimed) {
		return claimed;
	}

	if(event.type == SDL_KEYDOWN) {
		if(event.key.keysym.sym == SDLK_LEFT) {
			select(selected_-1);
			claimed = true;
		} else if(event.key.keysym.sym == SDLK_RIGHT) {
			select(selected_+1);
			claimed = true;
		} else if(event.key.keysym.sym == SDLK_RETURN ||
		          event.key.keysym.sym == SDLK_SPACE) {
			selection_made_ = true;
			claimed = true;
		}
	}
	return claimed;
}

void battle_menu::select(int num)
{
	while(num < 0) {
		num += menu_.size();
	}

	num = num%menu_.size();

	if(num == selected_) {
		return;
	}

	selected_ = num;

	grid_.reset(new grid(2));
	const submenu& menu = menu_[selected_];
	foreach(game_logic::const_battle_move_ptr option, menu.options) {
		grid_->add_col(widget_ptr(new image_widget(menu.tex,50,50))).
		       add_col(widget_ptr(new label(option->name(), graphics::color_yellow(),20)));
	}

	grid_->must_select();
	grid_->set_loc(x() + selected_*category_size + category_size/2 - grid_->width()/2, y() - grid_->height());
}

}
