#ifndef TIME_COST_WIDGET_HPP_INCLUDED
#define TIME_COST_WIDGET_HPP_INCLUDED

#include "boost/shared_ptr.hpp"
#include "location_tracker.hpp"

namespace game_dialogs {

class time_cost_widget: public gui::widget {
public:
	time_cost_widget() :
		tracker_(NULL), time_cost_(0)
	{
		set_visible(false);
	}
	void set_tracker(const graphics::location_tracker* tracker) { tracker_ = tracker; }
	void clear_tracker() { tracker_ = NULL; }
	void set_time_cost(int time_cost) { time_cost_ = time_cost; }
private:
	void handle_draw() const;
	const graphics::location_tracker* tracker_;
	int time_cost_;
};

typedef boost::shared_ptr<time_cost_widget> time_cost_widget_ptr;

}

/* impl is in status_bars_widget.cpp for convenience */

#endif
