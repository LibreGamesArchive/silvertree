
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#ifndef ITEM_HPP_INCLUDED
#define ITEM_HPP_INCLUDED

#include <string>

#include "formula_callable.hpp"
#include "item_fwd.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class equipment;

class item : public formula_callable
{
public:
	static void initialize(const wml::const_node_ptr& node);
	static const_item_ptr get(const std::string& id);
	static item_ptr create_item(const wml::const_node_ptr& node);
	item(ITEM_TYPE type, const wml::const_node_ptr& node);
	explicit item(ITEM_TYPE type);
	virtual ~item() {}

	virtual item_ptr clone() const { return item_ptr(new item(*this)); }
	
	const std::string& id() const { return id_; }
	ITEM_TYPE type() const { return type_; }
	const std::string& item_class() const { return class_; }
	const std::string& description() const { return description_; }
	const std::string& image() const { return image_; }
	bool is_null() const { return null_item_; }
	int value() const { return value_; }

	static const std::string& type_name(ITEM_TYPE type);
private:
	virtual variant get_value(const std::string& key) const;
	void get_inputs(std::vector<formula_input>* inputs) const;
	std::string id_;
	ITEM_TYPE type_;
	std::string class_;
	std::string description_;
	std::string image_;
	bool null_item_;
	int value_;
};

const equipment* item_as_equipment(const const_item_ptr& i);

}

#endif
