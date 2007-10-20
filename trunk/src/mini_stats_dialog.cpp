#include <math.h>
#include <sstream>

#include "character.hpp"
#include "raster.hpp"
#include "image_widget.hpp"
#include "mini_stats_dialog.hpp"
#include "label.hpp"

namespace game_dialogs {

mini_stats_dialog::mini_stats_dialog(game_logic::battle_character_ptr ch, int w, int h)
	: framed_dialog(0,0,w,h), ch_(ch)
{
  construct_interface();
}

void mini_stats_dialog::prepare_draw() const {
	GLfloat box[4];
	ch_->loc_tracker().get_bbox(box);

	glPushMatrix();
	glTranslatef(floor(box[2]), floor(box[1]), 0);
}

void mini_stats_dialog::finish_draw() const {
	glPopMatrix();
}

void mini_stats_dialog::construct_interface()
{
	game_logic::character& rch = ch_->get_character();

	std::string portrait_file = rch.portrait();
	if(!portrait_file.empty()) {
		gui::widget_ptr portrait =
			gui::widget_ptr(new gui::image_widget(portrait_file, width()/2, height()));
		add_widget(portrait, dialog::MOVE_RIGHT);
	} else {
		set_dim(width()/2, height());
	}

	gui::label_ptr stats_label(new gui::label(ch_->status_text(), graphics::color_white()));
	add_widget(stats_label);
}
}
