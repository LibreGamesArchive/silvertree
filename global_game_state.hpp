#ifndef GLOBAL_GAME_STATE_HPP_INCLUDED
#define GLOBAL_GAME_STATE_HPP_INCLUDED

#include <string>

#include "wml_node_fwd.hpp"

class variant;

namespace game_logic {

class formula_callable;

class global_game_state {
public:
	static global_game_state& get();
	void reset();
	void init(wml::const_node_ptr node);
	void write(wml::node_ptr node) const;

	const variant& get_variable(const std::string& varname) const;
	void set_variable(const std::string& varname, const variant& value);

	const formula_callable& get_variables() const;

private:
	global_game_state();
};

}

#endif
