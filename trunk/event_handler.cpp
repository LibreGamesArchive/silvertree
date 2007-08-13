#include <iostream>

#include "event_handler.hpp"
#include "foreach.hpp"
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

void event_handler::handle(const formula_callable& info, world& world)
{
	foreach(const formula_ptr& f, filters_) {
		if(!f->execute(info).as_bool()) {
			return;
		}
	}

	for(wml::node::const_all_child_iterator i = node_->begin_children();
	    i != node_->end_children(); ++i) {
		const wml::const_node_ptr& item = *i;
		if(item->name() == "destroy_party") {
			const std::string& filter_str = (*item)["filter"];
			formula_ptr filter;
			if(!filter_str.empty()) {
				filter.reset(new formula(filter_str));
			}
			std::vector<party_ptr> parties;
			world.get_matching_parties(filter,parties);
			foreach(const party_ptr& p, parties) {
				p->destroy();
			}
		}
	}
}

void event_handler::add_filter(formula_ptr f)
{
	filters_.push_back(f);
}

}
