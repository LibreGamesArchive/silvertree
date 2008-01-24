#ifndef MINI_STATS_DIALOG_HPP_INCLUDED
#define MINI_STATS_DIALOG_HPP_INCLUDED

#include "boost/shared_ptr.hpp"
#include <GL/glew.h>

#include "battle_character.hpp"
#include "gui_core.hpp"

namespace game_dialogs {

class mini_stats_dialog : public gui::framed_dialog {
public:
	mini_stats_dialog(game_logic::battle_character_ptr ch, int w=100, int h=100);
	void set_loc(int x, int y) {}
protected:
	void construct_interface();
private:
	void prepare_draw() const;
	void finish_draw() const;
	void update_location();

	game_logic::battle_character_ptr ch_;
};

typedef boost::shared_ptr<mini_stats_dialog> mini_stats_dialog_ptr;

}


#endif
