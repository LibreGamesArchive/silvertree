#include "character.hpp"
#include "foreach.hpp"
#include "formatter.hpp"
#include "grid_widget.hpp"
#include "image_widget.hpp"
#include "party.hpp"
#include "post_battle_dialog.hpp"

namespace game_dialogs {

post_battle_dialog::post_battle_dialog(game_logic::party_ptr p,
                                       int xp, int money)
    : dialog(0,0,1024,768), party_(p), xp_(xp), money_(money)
{
	using namespace gui;
	typedef widget_ptr ptr;
	const SDL_Color color = {0xFF,0xFF,0,0xFF};
	set_padding(30);

	const int sz = 22;
	label_factory lb(color, sz);

	money_label_ = lb.create(formatter() << party_->money());
	money_left_ = lb.create(formatter() << money_);
	xp_left_ = lb.create(formatter() << xp_);
	grid_ptr grid(new gui::grid(4));
	grid->set_hpad(20).
	      add_col(lb.create("currency_units")).
	      add_col(money_label_).
		  add_col(lb.create("currency_found")).
		  add_col(money_left_).
		  add_col().
		  add_col().
		  add_col(lb.create("experience_awarded")).
		  add_col(xp_left_);
	add_widget(grid, 20, 20);

	grid.reset(new gui::grid(2));
	foreach(game_logic::character_ptr c, party_->members()) {
		grid->add_col(ptr(new image_widget(c->portrait(),100,100)));

		label_ptr level_label(lb.create(formatter() << c->level()));
		label_ptr xp_label(lb.create(formatter() <<
		                     (c->experience_required() - c->experience())));

		level_.push_back(level_label);
		xp_required_.push_back(xp_label);

		grid_ptr stats_grid(new gui::grid(2));
		stats_grid->set_hpad(20).
				    add_col(lb.create(c->description())).finish_row().
					add_col(lb.create("level")).
					add_col(level_label).
					add_col(lb.create("experience_required")).
					add_col(xp_label);
		grid->add_col(stats_grid);
	}

	add_widget(grid);
}

bool post_battle_dialog::iterate()
{
	if(money_) {
		--money_;
		party_->get_money(1);
		money_label_->set_text(formatter() << party_->money());
		money_left_->set_text(formatter() << money_);
		return true;
	} else if(xp_) {
		--xp_;
		xp_left_->set_text(formatter() << xp_);
		int index = 0;
		foreach(game_logic::character_ptr c, party_->members()) {
			c->award_experience(1);
			xp_required_[index]->set_text(formatter() <<
			               (c->experience_required() - c->experience()));
			level_[index]->set_text(formatter() << c->level());
			++index;
		}
		return true;
	} else {
		return false;
	}
}

void post_battle_dialog::show()
{
	prepare_draw();
	draw();
	complete_draw();
	SDL_Delay(2000);

	while(iterate()) {
		prepare_draw();
		draw();
		complete_draw();
		SDL_Delay(10);
	}

	SDL_Delay(1000);
}

}
