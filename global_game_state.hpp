#ifndef GLOBAL_GAME_STATE_HPP_INCLUDED
#define GLOBAL_GAME_STATE_HPP_INCLUDED

#include <string>

#include "wml_node_fwd.hpp"

class variant;

namespace game_logic {

class formula_callable;

struct event_context_internal;

class global_game_state {
public:
	static global_game_state& get();
	void reset();
	void init(wml::const_node_ptr node);
	void write(wml::node_ptr node) const;

	int generate_new_party_id() const;

	const variant& get_variable(const std::string& varname) const;
	void set_variable(const std::string& varname, const variant& value);

	const formula_callable& get_variables() const;

	struct event_context {
		event_context();
		~event_context();
		event_context_internal* impl_;
	private:
		event_context(const event_context&);
		void operator=(const event_context&);
	};

private:
	global_game_state();
};

}

#endif
