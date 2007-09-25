#include <iostream>
#include <vector>

#include "foreach.hpp"
#include "formula.hpp"
#include "party.hpp"
#include "wml_command.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "world.hpp"

namespace game_logic {

namespace {

class debug_command : public wml_command {
	std::string text_;
	void do_execute(const formula_callable& info, world& world) const {
		std::cerr << "DEBUG: " << text_ << "\n";
	}
public:
	explicit debug_command(wml::const_node_ptr node) : text_(node->attr("text"))
	{}
};

class destroy_party_command : public wml_command {
	formula_ptr filter_;
	void do_execute(const formula_callable& info, world& world) const {
		std::vector<party_ptr> parties;
		world.get_matching_parties(filter_.get(), parties);
		foreach(const party_ptr& p, parties) {
			p->destroy();
		}
	}
public:
	explicit destroy_party_command(wml::const_node_ptr node)
	{
		const std::string& filter = node->attr("filter");
		if(filter.empty() == false) {
			filter_.reset(new formula(filter));
		}
	}
};

class if_command : public wml_command {
	formula condition_;
	std::vector<const_wml_command_ptr> then_;
	std::vector<const_wml_command_ptr> else_;
	void do_execute(const formula_callable& info, world& world) const {
		const std::vector<const_wml_command_ptr>& commands =
		    condition_.execute(info).as_bool() ? then_ : else_;
		foreach(const const_wml_command_ptr& cmd, commands) {
			cmd->execute(info, world);
		}
	}
public:
	explicit if_command(wml::const_node_ptr node) : condition_(node->attr("condition"))
	{
		wml::const_node_ptr then = node->get_child("then");
		if(then) {
			for(wml::node::const_all_child_iterator i = then->begin_children();
			    i != then->end_children(); ++i) {
				then_.push_back(wml_command::create(*i));
			}
		}

		wml::const_node_ptr else_branch = node->get_child("else");
		if(then) {
			for(wml::node::const_all_child_iterator i = else_branch->begin_children();
			    i != else_branch->end_children(); ++i) {
				else_.push_back(wml_command::create(*i));
			}
		}
	}
};

class scripted_moves_command : public wml_command {
	formula filter_;
	std::vector<hex::location> locs_;
	void do_execute(const formula_callable& info, world& world) const {
		std::vector<party_ptr> parties;
		world.get_matching_parties(&filter_, parties);
		foreach(const party_ptr& p, parties) {
			foreach(const hex::location& loc, locs_) {
				p->add_scripted_move(loc);
			}
		}
	}
public:
	explicit scripted_moves_command(wml::const_node_ptr node) : filter_(node->attr("filter"))
	{
		wml::node::const_child_range locs = node->get_child_range("loc");
		while(locs.first != locs.second) {
			const wml::const_node_ptr loc = locs.first->second;
			locs_.push_back(hex::location(wml::get_int(loc,"x"),wml::get_int(loc,"y")));
			++locs.first;
		}
	}
};

class execute_script_command : public wml_command {
	std::string script_;
	void do_execute(const formula_callable& info, world& world) const {
		world.set_script(script_);
	}
public:
	explicit execute_script_command(wml::const_node_ptr node)
	   : script_(node->attr("script"))
	{
	}
};

}

const_wml_command_ptr wml_command::create(wml::const_node_ptr node)
{
#define DEFINE_COMMAND(cmd) \
	if(node->name() == #cmd) { \
		return const_wml_command_ptr(new cmd##_command(node)); \
	}

	DEFINE_COMMAND(debug);
	DEFINE_COMMAND(destroy_party);
	DEFINE_COMMAND(if);
	DEFINE_COMMAND(scripted_moves);
	DEFINE_COMMAND(execute_script);

	return const_wml_command_ptr();
}

}
