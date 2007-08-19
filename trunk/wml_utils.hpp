
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef WML_UTILS_HPP_INCLUDED
#define WML_UTILS_HPP_INCLUDED

#include "foreach.hpp"
#include "wml_node.hpp"

#include <boost/lexical_cast.hpp>

#define WML_READ_VECTOR(node, v, create_statement, element, ptr) \
	{ \
		wml::node::const_child_range range = node->get_child_range(element); \
		while(range.first != range.second) { \
			v.push_back(ptr(create_statement(range.first->second))); \
			++range.first; \
		} \
	}

namespace wml {

struct error {};

typedef std::vector<const_node_ptr> const_node_vector;
typedef std::vector<node_ptr> node_vector;

node_ptr deep_copy(const_node_ptr node);
void copy_over(const_node_ptr src, node_ptr dst);

std::vector<const_node_ptr> child_nodes(const const_node_ptr& ptr,
                                        const std::string& element);
std::vector<node_ptr> child_nodes(const node_ptr& ptr,
                                  const std::string& element);

inline const std::string& get_str(const_node_ptr ptr,
                                  const std::string& key)
{
	return (*ptr)[key];
}

inline bool get_bool(const_node_ptr ptr, const std::string& key,
                     bool default_val=false)
{
	const std::string& str = get_str(ptr,key);
	if(str.empty()) {
		return default_val;
	}
	return str == "yes" || str == "true";
}

template<typename T>
T get_attr(const_node_ptr ptr, const std::string& key,
           T default_value=T())
{
	if(!ptr) {
		return default_value;
	}
	
	try {
		return boost::lexical_cast<T>((*ptr)[key]);
	} catch(boost::bad_lexical_cast&) {
		return default_value;
	}
}

inline int get_int(const_node_ptr ptr, const std::string& key,
                   int default_value=0)
{
	return get_attr<int>(ptr,key,default_value);
}

inline bool require(bool cond)
{
	if(!cond) {
		throw error();
	}
}

}

#endif
