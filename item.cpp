
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include "foreach.hpp"
#include "equipment.hpp"
#include "item.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace game_logic
{

namespace {
const std::string type_names[] = {
"none", "weapon", "shield", "armor", "helmet",
};

std::map<std::string,const_item_ptr> registry;
}

void item::initialize(const wml::const_node_ptr& node)
{
	wml::const_node_vector items = wml::child_nodes(node, "item");
	foreach(wml::const_node_ptr i, items) {
		registry[(*i)["id"]] = create_item(i);
	}
}

const_item_ptr item::get(const std::string& id)
{
	return registry[id];
}

item_ptr item::create_item(const wml::const_node_ptr& node)
{
	ITEM_TYPE type = ITEM_NONE;
	const std::string& attr_type = wml::get_str(node, "type");
	const std::string& type_str = attr_type.empty() ? node->name() : attr_type;

	for(int n = 0; n != sizeof(type_names)/sizeof(*type_names); ++n) {
		if(type_names[n] == type_str) {
			type = ITEM_TYPE(n);
			break;
		}
	}

	if(type >= ITEM_TYPE(BEGIN_EQUIPMENT)) {
		return item_ptr(new equipment(type, node));
	} else {
		return item_ptr(new item(type, node));
	}
}

item::item(ITEM_TYPE type, const wml::const_node_ptr& node)
  : id_(wml::get_str(node,"id")), type_(type), class_(wml::get_str(node,"class")),
    description_(wml::get_str(node,"description")),
	image_(wml::get_str(node,"image")),
	null_item_(wml::get_bool(node,"none")),
	value_(wml::get_int(node,"value"))
{
}

item::item(ITEM_TYPE type) : type_(type), description_("None"),
                             null_item_(true)
{
}

const equipment* item_as_equipment(const const_item_ptr& i)
{
	return dynamic_cast<const equipment*>(i.get());
}

const std::string& item::type_name(ITEM_TYPE type)
{
	return type_names[type];
}

variant item::get_value(const std::string& key) const
{
	return variant();
}

}
