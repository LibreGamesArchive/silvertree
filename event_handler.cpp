#include <iostream>

#include "event_handler.hpp"
#include "foreach.hpp"
#include "global_game_state.hpp"
#include "party.hpp"
#include "wml_command.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "world.hpp"

namespace game_logic
{

event_handler::event_handler(wml::const_node_ptr node)
    : node_(node),
	  first_time_only_(wml::get_bool(node, "first_time_only", false)),
	  already_run_(wml::get_bool(node, "already_run", false))
{
	const std::string& f = wml::get_str(node,"filter");
	if(!f.empty()) {
		filters_.push_back(formula_ptr(new formula(f)));
	}

	for(wml::node::const_all_child_iterator i = node->begin_children();
	    i != node->end_children(); ++i) {
		if((*i)->name() == "filter") {
			filters_.push_back(formula_ptr(new formula((*i)->attr("filter"))));
			continue;
		}

		const_wml_command_ptr cmd(wml_command::create(*i));
		if(cmd) {
			commands_.push_back(cmd);
		}
	}
}

void event_handler::handle(const formula_callable& info, world& world)
{
	if(first_time_only_ && already_run_) {
		return;
	}

	foreach(const formula_ptr& f, filters_) {
		if(!f->execute(info).as_bool()) {
			return;
		}
	}

	already_run_ = true;

	foreach(const const_wml_command_ptr& cmd, commands_) {
		cmd->execute(info, world);
	}
}

void event_handler::add_filter(formula_ptr f)
{
	filters_.push_back(f);
}

wml::const_node_ptr event_handler::write() const
{
	if(first_time_only_ && already_run_) {
		return wml::const_node_ptr();
	} else {
		return node_;
	}
}

}
