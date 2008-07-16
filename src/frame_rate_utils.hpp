
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef FRAME_SKIPPER_HPP_INCLUDED
#define FRAME_SKIPPER_HPP_INCLUDED

#include <iostream>
#include <string>
#include <sstream>
#include "SDL.h"

namespace graphics {

class frame_skipper {
public:
    frame_skipper(int rate=50, bool maxfps=false)
        : maxfps_(maxfps), frame_time_(1000/rate),
          last_(0), accum_(0), 
          skipped_last_frame_(false)
    {
        last_ = SDL_GetTicks();
    }
    bool skip_frame();
    void reset();
private:
    const bool maxfps_;
    const int frame_time_;
    int last_;
    int accum_;
    bool skipped_last_frame_;
};

inline bool frame_skipper::skip_frame() {
    const int start = SDL_GetTicks();
    bool ret = false;
    int elapsed = 0;
    
    if(!skipped_last_frame_) {
        elapsed = start - last_;
    }
    accum_ += (elapsed - frame_time_);

    if(!maxfps_) {
        if(accum_ < 0) {
            SDL_Delay(-accum_);
        } else {
            SDL_Delay(1);
        }
    }

    ret = accum_ > frame_time_;
    skipped_last_frame_ = ret;
    last_ = start;
    return ret;
}

inline void frame_skipper::reset() {
	last_ = SDL_GetTicks();
	accum_ = 0;
}

class frame_rate_tracker {
public:
	frame_rate_tracker()
		: last_tick_(0), last_frames_(0), frames_(0),
		  fps_(50), msg_up_to_date_(false)
	{
		last_tick_ = SDL_GetTicks();
	}
	void register_frame(bool drawn);
	int frame_rate() const;
	const std::string& msg() const;
	void reset();
private:
	Uint32 last_tick_, last_frames_, frames_, fps_;
	mutable std::string fps_msg_;
	mutable bool msg_up_to_date_;
};

inline void frame_rate_tracker::register_frame(bool drawn)
{
	const int now = SDL_GetTicks();

	if(drawn) {
		++frames_;
	}

	const int elapsed = now - last_tick_;
	if(elapsed > 1000) {
		const int frames_elapsed = frames_ - last_frames_;
		last_frames_ = frames_;
		last_tick_ = now;
		fps_ = static_cast<int>((frames_elapsed*1000.0) / elapsed);
		msg_up_to_date_ = false;
	}
}

inline int frame_rate_tracker::frame_rate() const
{
	return fps_;
}

inline const std::string& frame_rate_tracker::msg() const
{
	if(!msg_up_to_date_) {
		std::ostringstream stream;
		stream << fps_ << "fps";
		fps_msg_ = stream.str();
		msg_up_to_date_ =true;
	}
	return fps_msg_;
}

inline void frame_rate_tracker::reset() {
	frames_ = 0;
	last_frames_ = 0;
	last_tick_ = SDL_GetTicks();
	fps_ = 50;
	msg_up_to_date_ = false;
}

}
#endif
