#include <cassert>
#include <iostream>
#include <map>
#include <string>

#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "global_game_state.hpp"
#include "variant.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace game_logic {

namespace {
int current_party_id = 1;
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

	void set_value(const std::string& key, const variant& value) {
		variables[key] = value;
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

	current_party_id = wml::get_int(node, "current_party_id", 1);
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

	node->set_attr("current_party_id", formatter() << current_party_id);
}

global_game_state::global_game_state()
{
	static bool first_and_only_time = true;
	assert(first_and_only_time);
	first_and_only_time = false;
}

int global_game_state::generate_new_party_id() const
{
	return current_party_id++;
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
	variables[varname] = value;
}

const formula_callable& global_game_state::get_variables() const
{
	static bool first_time = true;
	static variables_callable callable;

	//make sure the callable will never run out of references by giving it one to begin with
	if(first_time) {
		first_time = false;
		callable.add_ref();
	}
	return callable;
}

}
