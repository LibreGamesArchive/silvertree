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
	void set_loc(int x, int y);
	void set_dim(int w, int h);
protected:
	framed_dialog(int x, int y, int w, int h) 
		: dialog(x,y,w,h), 
		  nested_draw_(false), nested_set_dim_(false), nested_set_loc_(false) {}
	virtual ~framed_dialog() {}
private:
	void handle_draw() const;
	virtual void inner_set_dim(int w, int h);
	virtual void inner_set_loc(int x, int y);
	virtual void prepare_draw() const =0;
	virtual void inner_draw() const =0;
	virtual void finish_draw() const =0;

	frame_ptr frame_;
	mutable bool nested_draw_, nested_set_dim_, nested_set_loc_;
};
}

namespace game_dialogs {

class mini_stats_dialog : public gui::framed_dialog {
public:
	mini_stats_dialog(game_logic::battle_character_ptr ch, int w=100, int h=100);
	void set_loc(int x, int y) {}
protected:
	void construct_interface();		
private:
	void prepare_draw() const;
	void inner_draw() const;
	void finish_draw() const;
	void update_location();

	game_logic::battle_character_ptr ch_;
};

typedef boost::shared_ptr<mini_stats_dialog> mini_stats_dialog_ptr;

}


#endif
