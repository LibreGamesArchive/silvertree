/***************************************************************************
 *  Copyright (C) 2008 by Sergey Popov <loonycyborg@gmail.com>             *
 *                                                                         *
 *  This file is part of Silver Tree.                                      *
 *                                                                         *
 *  Silver Tree is free software; you can redistribute it and/or modify    *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  Silver Tree is distributed in the hope that it will be useful,         *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#ifndef ANIMATION_HPP_INCLUDED
#define ANIMATION_HPP_INCLUDED

#include <boost/scoped_ptr.hpp>
#include <boost/utility.hpp>

#include <SDL.h>

namespace graphics {

class time_source : boost::noncopyable
{
	Uint32 ticks_;

	time_source() : ticks_(SDL_GetTicks()) {}

	static boost::scoped_ptr<time_source> time_source_;

	public:
	void advance_time() { ticks_ = SDL_GetTicks(); }
	Uint32 time() const { return ticks_; }

	static time_source& instance()
	{
		if(!time_source_)
			time_source_.reset(new time_source);
		return *time_source_;
	}
};

template <class ValueType> class animation
{
	Uint32 start_;
	float duration_;

	float time_offset() const { return static_cast<float>(time_source::instance().time() - start_)/1000.0; }
	public:
	animation() : start_(time_source::instance().time()), duration_(0) {}
	virtual ~animation() {}
	virtual ValueType operator()(float time) const = 0;

	ValueType play() const { return (*this)(time_offset()); }
	void set_duration(float duration) { duration_ = duration; }
	bool finished() const { return time_offset() > duration_; }
};

template <class ValueType, class Function> class function_animation : public animation<ValueType>
{
	Function function_;
	public:
	function_animation() : function_() {}
	function_animation(Function function) : function_(function) {}
	ValueType operator()(float time) const { return function_(time); }
};

template <class ValueType> class linear_function
{
	ValueType a_, b_;

	public:
	linear_function(ValueType a, ValueType b) : a_(a), b_(b) {}
	ValueType operator()(float x) const { return a_ * x + b_; }
};

template <class ValueType, class Function> function_animation<ValueType, Function> make_animation(Function function)
{
	return function_animation<ValueType, Function>(function);
}

template <class ValueType> class quadratic_function
{
	ValueType a_, b_, c_;

	public:
	quadratic_function() : a_(), b_(), c_() {}
	quadratic_function(ValueType a, ValueType b, ValueType c) : a_(a), b_(b), c_(c) {}
	ValueType operator()(float x) const { return a_ * (x * x) + b_ * x + c_; }
};

}

#endif
