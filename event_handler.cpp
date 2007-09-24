#include <iostream>

#include "event_handler.hpp"
#include "foreach.hpp"
#include "global_game_state.hpp"
#include "party.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "world.hpp"

namespace game_logic
{

event_handler::event_handler(wml::const_node_ptr node) : node_(node)
{
	const std::string& f = wml::get_str(node,"filter");
	if(!f.empty()) {
		filters_.push_back(formula_ptr(new formula(f)));
	}
}

namespace {

void execute_command(const wml::const_node_ptr& cmd, const formula_callable& info, world& world)
{
	if(cmd->name() == "debug") {
		std::cerr << cmd->attr("text") << "\n";
	} else if(cmd->name() == "set") {
		for(wml::node::const_attr_iterator i = cmd->begin_attr(); i != cmd->end_attr(); ++i) {
			formula::executor exec(formula(i->second), &info);
			global_game_state::get().set_variable(i->first, exec.result());
		}
	} else if(cmd->name() == "destroy_party") {
		const std::string& filter_str = (*cmd)["filter"];
		formula_ptr filter;
		if(!filter_str.empty()) {
			filter.reset(new formula(filter_str));
		}
		std::vector<party_ptr> parties;
		world.get_matching_parties(filter,parties);
		foreach(const party_ptr& p, parties) {
			p->destroy();
		}
	} else if(cmd->name() == "if") {
		const std::string branch_name = formula(cmd->attr("condition")).execute(info).as_bool() ?
		                           "then" : "else";
		wml::const_node_ptr branch = cmd->get_child(branch_name);
		if(branch) {
			for(wml::node::const_all_child_iterator i = cmd->begin_children();
			    i != cmd->end_children(); ++i) {
				execute_command(*i, info, world);
			}
		}
	}
}

}

void event_handler::handle(const formula_callable& info, world& world)
{
	foreach(const formula_ptr& f, filters_) {
		if(!f->execute(info).as_bool()) {
			return;
		}
	}

	for(wml::node::const_all_child_iterator i = node_->begin_children();
	    i != node_->end_children(); ++i) {
		execute_command(*i, info, world);
	}
}

void event_handler::add_filter(formula_ptr f)
{
	filters_.push_back(f);
}

}
