#include <cassert>
#include <iostream>
#include <map>
#include <string>

#include "formula.hpp"
#include "global_game_state.hpp"
#include "variant.hpp"
#include "wml_node.hpp"

namespace game_logic {

namespace {
typedef std::map<std::string, variant> variable_map;
variable_map variables;

class variables_callable : public formula_callable
{
private:
	variant get_value(const std::string& key) const {
		variable_map::const_iterator i = variables.find(key);
		if(i != variables.end()) {
			return i->second;
		} else {
			return variant();
		}
	}
};
}

global_game_state& global_game_state::get()
{
	static global_game_state state;
	return state;
}

void global_game_state::reset()
{
	variables.clear();
}

void global_game_state::init(wml::const_node_ptr node)
{
	reset();
	wml::const_node_ptr var = node->get_child("variables");
	if(var) {
		for(wml::node::const_attr_iterator i = var->begin_attr(); i != var->end_attr(); ++i) {
			variant v;
			v.serialize_from_string(i->second);
			variables[i->first] = v;
		}
	}
}

void global_game_state::write(wml::node_ptr node) const
{
	wml::node_ptr var(new wml::node("variables"));
	for(variable_map::const_iterator i = variables.begin(); i != variables.end(); ++i) {
		std::string val;
		i->second.serialize_to_string(val);
		var->set_attr(i->first, val);
	}

	node->add_child(var);
}

global_game_state::global_game_state()
{
	static bool first_and_only_time = true;
	assert(first_and_only_time);
	first_and_only_time = false;
}

const variant& global_game_state::get_variable(const std::string& varname) const
{
	const variable_map::const_iterator i = variables.find(varname);
	if(i == variables.end()) {
		static variant null_variant;
		return null_variant;
	}

	return i->second;
}

void global_game_state::set_variable(const std::string& varname, const variant& value)
{
	//if the old value of the variant holds resources, free them. Deep copy the resources of
	//the new variant into the existing one.
	variant& var = variables[varname];
	var.release_resources();
	var = value.deep_copy();
}

const formula_callable& global_game_state::get_variables() const
{
	static variables_callable callable;
	return callable;
}

}
