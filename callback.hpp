
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef CALLBACK_HPP_INCLUDED
#define CALLBACK_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace functional
{

class callback_base
{
public:
	void call() { do_call(); }
private:
	virtual void do_call() = 0;
};

typedef boost::shared_ptr<callback_base> callback_ptr;

template<typename T>
class callback : public callback_base
{
public:
	explicit callback(T* ptr) : ptr_(ptr)
	{}
	virtual ~callback() {}
private:
	void do_call() { execute(ptr_); }
	virtual void execute(T* ptr) = 0;

	T* ptr_;
};

template<typename Arg>
class callback_base_arg1
{
public:
	void call(const Arg& arg) { do_call(arg); }
private:
	virtual void do_call(const Arg& arg) = 0;
};

template<typename Arg>
struct callback_arg {
	typedef boost::shared_ptr<callback_base_arg1<Arg> > ptr;
};

template<typename T, typename Arg>
class callback_arg1 : public callback_base_arg1<Arg>
{
public:
	explicit callback_arg1(T* ptr) : ptr_(ptr)
	{}
	virtual ~callback_arg1() {}
private:
	void do_call(const Arg& arg) { execute(ptr_,arg); }
	virtual void execute(T* ptr, const Arg& arg) = 0;

	T* ptr_;
};

#define DEFINE_CALLBACK(name, type, function) \
class name : public functional::callback<type> \
{ \
	void execute(type* ptr) { ptr->function(); } \
public: \
	explicit name(type* ptr) : functional::callback<type>(ptr) {} \
};

#define DEFINE_CALLBACK_ARG1(name, type, arg_type, function) \
class name : public functional::callback_arg1<type,arg_type> \
{ \
	void execute(type* ptr, const arg_type& arg) { ptr->function(arg); } \
public: \
	name(type* ptr) : functional::callback_arg1(ptr) {} \
};

}

#endif
