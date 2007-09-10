#ifndef MINI_STATS_DIALOG_HPP_INCLUDED
#define MINI_STATS_DIALOG_HPP_INCLUDED

#include "SDL.h"
#include "boost/shared_ptr.hpp"
#include <gl.h>

#include "battle_character.hpp"
#include "dialog.hpp"
#include "frame.hpp"
#include "label.hpp"

namespace gui {

class framed_dialog: public dialog {
public:
	void set_frame(frame_ptr frame) { frame_ = frame; }
	const frame_ptr get_frame() { return frame_; }
	void set_dim(int w, int h) {}
protected:
	framed_dialog(int x, int y, int w, int h) 
		: dialog(x,y,w,h), nested_(false) {}
	virtual ~framed_dialog() {}
private:
	void handle_draw() const;
	virtual void prepare_draw() const =0;
	virtual void inner_draw() const =0;
	virtual void finish_draw() const =0;

	frame_ptr frame_;
	mutable bool nested_;
};
}

namespace game_dialogs {

class mini_stats_dialog : public gui::framed_dialog {
public:
	mini_stats_dialog(game_logic::battle_character_ptr ch, 
			  int ttl=-1, int w=100, int h=100);
	void set_loc(int x, int y) {}
	void refresh();
protected:
	void construct_interface();		
private:
	bool hits_me(const SDL_Event& e);
	void check_death() const;
	void prepare_draw() const;
	void inner_draw() const;
	void finish_draw() const;
	void handle_event(const SDL_Event& e);
	void update_location();
	void hide_and_close();

	game_logic::battle_character_ptr ch_;
	gui::label_ptr health_label_;
	int ttl_, death_time_;
	mutable bool dead_;
};

typedef boost::shared_ptr<mini_stats_dialog> mini_stats_dialog_ptr;

}


#endif
