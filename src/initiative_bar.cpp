#include <algorithm>

#include "battle_character.hpp"
#include "character.hpp"
#include "foreach.hpp"
#include "initiative_bar.hpp"
#include "texture.hpp"

namespace gui {

void initiative_bar::add_character(const game_logic::const_battle_character_ptr& c)
{
	chars_.push_back(c);
}

void initiative_bar::remove_character(const game_logic::battle_character* c)
{
	for(std::vector<game_logic::const_battle_character_ptr>::iterator i = chars_.begin(); i != chars_.end(); ++i) {
		if(i->get() == c) {
			chars_.erase(i);
			break;
		}
	}
}

void initiative_bar::focus_character(const game_logic::battle_character* c, double add)
{
	focus_ = c;
	add_focus_ = add;
}

void initiative_bar::handle_draw() const
{
	const int ImageSize = 50;
	const int LineLength = 10;

	double max_time = -1;
	foreach(const game_logic::const_battle_character_ptr& c, chars_) {
		const int time = c->ready_to_move_at();

		if(max_time == -1 || time > max_time) {
			max_time = time;
		}
	}

	if(max_time - current_time_ < 50) {
		max_time = current_time_ + 50;
	}

	const int min_pos = y() + ImageSize/2;
	const int max_pos = y() + height() - ImageSize/2;

	const GLfloat time_scale = max_time - current_time_;
	const SDL_Rect scale_rect = {x() + width()/2, min_pos, 1, max_pos - min_pos};
	graphics::draw_rect(scale_rect, graphics::color_white(), 127);
	for(double time = 0.0; time <= max_time - current_time_; time += 5.0) {
		const SDL_Rect tick_rect = { x() + width()/2 - 3, static_cast<int>(min_pos + (time/time_scale)*(max_pos-min_pos)), 7, 1};

		graphics::draw_rect(tick_rect, graphics::color_white(), 127);
	}

	foreach(const game_logic::const_battle_character_ptr& bc, chars_) {
		const game_logic::character& ch = bc->get_character();
		const std::string& image = ch.portrait().empty() ? ch.image() : ch.portrait();
		graphics::texture tex(graphics::texture::get(image));

		const GLfloat time = bc->ready_to_move_at() + (bc.get() == focus_ ? add_focus_ : 0.0) - current_time_;
		const int pos = static_cast<int>(min_pos + (time/time_scale)*(max_pos-min_pos));

		//the pc characters get drawn on the left, enemies on the right
		const bool human = bc->get_party().is_human_controlled();
		const SDL_Rect line_rect = { x() + width()/2 + (human ? -LineLength : 0),
		                             pos, LineLength, 1 };
		graphics::draw_rect(line_rect, human ? graphics::color_blue() : graphics::color_red());


		graphics::blit_texture(tex, x() + width()/2 + (human ? -LineLength - ImageSize : LineLength), pos-ImageSize/2, width(), width());
	}
}

}
