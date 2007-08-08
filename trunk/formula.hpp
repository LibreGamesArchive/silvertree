
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef FORMULA_HPP_INCLUDED
#define FORMULA_HPP_INCLUDED

#include <map>
#include <string>

#include "formula_fwd.hpp"
#include "variant.hpp"

namespace game_logic
{

//interface for objects that can have formulae run on them
class formula_callable {
public:
	variant query_value(const std::string& key) const {
		return get_value(key);
	}

protected:
	~formula_callable() {}

private:
	virtual variant get_value(const std::string& key) const = 0;
};

class map_formula_callable : public formula_callable {
public:
	explicit map_formula_callable(const formula_callable* fallback=NULL);
	map_formula_callable& add(const std::string& key, const variant& value);
private:
	variant get_value(const std::string& key) const;
	std::map<std::string,variant> values_;
	const formula_callable* fallback_;
};

class formula_expression;
typedef boost::shared_ptr<formula_expression> expression_ptr;

class formula {
public:
	static variant evaluate(const const_formula_ptr& f,
	                    const formula_callable& variables,
						variant default_res=variant(0)) {
		if(f) {
			return f->execute(variables);
		} else {
			return default_res;
		}
	}
	explicit formula(const std::string& str);
	variant execute(const formula_callable& variables) const;

private:
	expression_ptr expr_;
};

struct formula_error
{
};

}

#endif
