#include <iostream>
#include <vector>

#include "foreach.hpp"
#include "formula.hpp"
#include "message_dialog.hpp"
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
	{}
};

class modify_objects_command : public wml_command {
	formula object_finder_;
	std::map<std::string,const_formula_ptr> modify_;
	void do_execute(const formula_callable& info, world& world) const {
		const variant objects = object_finder_.execute(info);
		map_formula_callable callable(&info);
		for(int n = 0; n != objects.num_elements(); ++n) {
			variant obj = objects[n];
			callable.add("object", obj);
			for(std::map<std::string,const_formula_ptr>::const_iterator i = modify_.begin(); i != modify_.end(); ++i) {
				obj.mutable_callable()->mutate_value(i->first, i->second->execute(callable));
			}
		}
	}
public:
	explicit modify_objects_command(wml::const_node_ptr node)
	   : object_finder_(node->attr("objects"))
	{
		for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
			if(i->first != "objects") {
				modify_[i->first].reset(new formula(i->second));
			}
		}
	}
};

class dialog_command : public wml_command {
	formula pc_formula_, npc_formula_;
	const_formula_ptr text_;
	std::vector<std::string> options_;
	std::vector<std::vector<const_wml_command_ptr> > consequences_;
	void do_execute(const formula_callable& info, world& world) const {
		const formula_callable* pc = pc_formula_.execute(info).as_callable();
		const formula_callable* npc = npc_formula_.execute(info).as_callable();
		const party* pc_party = dynamic_cast<const party*>(pc);
		const party* npc_party = dynamic_cast<const party*>(npc);
		if(!pc_party || !npc_party) {
			std::cerr << "Could not calculate parties in dialog\n";
			return;
		}

		gui::message_dialog dialog(*pc_party, *npc_party, text_->execute(info).as_string(),
		                           options_.empty() ? NULL : &options_,
								   false);
		dialog.show_modal();
		if(!options_.empty()) {
			const int option = dialog.selected();
			if(option >= 0 && option < consequences_.size()) {
				const std::vector<const_wml_command_ptr> cons = consequences_[option];
				foreach(const const_wml_command_ptr& c, cons) {
					c->execute(info, world);
				}
			}
		}
	}
public:
	explicit dialog_command(wml::const_node_ptr node)
	   : pc_formula_(wml::get_str(node, "pc", "pc")),
		 npc_formula_(wml::get_str(node, "npc", "npc")),
		 text_(formula::create_string_formula(wml::get_str(node, "text")))
	{
		wml::node::const_child_range options = node->get_child_range("option");
		while(options.first != options.second) {
			const wml::const_node_ptr op = options.first->second;
			options_.push_back(wml::get_str(op, "text"));
			std::vector<const_wml_command_ptr> consequences;
			for(wml::node::const_all_child_iterator j = op->begin_children(); j != op->end_children(); ++j) {
				const_wml_command_ptr cmd = create(*j);
				if(cmd) {
					consequences.push_back(cmd);
				}
			}
			consequences_.push_back(consequences);
			++options.first;
		}
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
	DEFINE_COMMAND(modify_objects);
	DEFINE_COMMAND(dialog);

	return const_wml_command_ptr();
}

}
