#ifndef BATTLE_MENU_HPP_INCLUDED
#define BATTLE_MENU_HPP_INCLUDED

#include <vector>

#include "battle.hpp"
#include "battle_character.hpp"
#include "battle_menu_fwd.hpp"
#include "battle_move_fwd.hpp"
#include "grid_widget_fwd.hpp"
#include "texture.hpp"
#include "widget.hpp"

namespace gui {

class battle_menu : public widget
{
public:
	battle_menu(const game_logic::battle& b,
	            const game_logic::battle_character& c);

	game_logic::const_battle_move_ptr selected_move() const;
	game_logic::const_battle_move_ptr highlighted_move() const;
private:
	void handle_draw() const;
	bool handle_event(const SDL_Event& event, bool claimed);
	void select(int num);

	const game_logic::battle& battle_;
	const game_logic::battle_character& char_;

	struct submenu {
		graphics::texture tex;
		std::string category;
		std::vector<game_logic::const_battle_move_ptr> options;
	};

	std::vector<submenu> menu_;
	int selected_;
	grid_ptr grid_;
	bool selection_made_;
};

}

#endif
