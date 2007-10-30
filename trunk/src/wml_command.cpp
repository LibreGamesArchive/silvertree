#include <iostream>
#include <vector>

#include "battle.hpp"
#include "battle_character.hpp"
#include "battle_map_generator.hpp"
#include "character_status_dialog.hpp"
#include "encounter.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "label.hpp"
#include "message_dialog.hpp"
#include "party.hpp"
#include "shop_dialog.hpp"
#include "wml_command.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"
#include "world.hpp"
#include "wml_writer.hpp"

namespace game_logic {

namespace {

class debug_console_command : public wml_command {
	void do_execute(const formula_callable& info, world& world) const {
		std::cerr << "starting debug console. Type formula to evaluate. Type 'continue' when you're ready to continue\n";
		std::cerr << variant(&info).to_debug_string() << "\n";
		for(;;) {
			std::cerr << "\n>>> ";
			char buf[1024];
			std::cin.getline(buf, sizeof(buf));
			std::string cmd(buf);
			if(cmd == "continue") {
				break;
			}

			try {
				formula f(cmd);
				const variant v = f.execute(info);
				std::cerr << v.to_debug_string() << "\n";
			} catch(formula_error& e) {
				std::cerr << "ERROR IN FORMULA\n";
			}
		}
	}
public:
	explicit debug_console_command(wml::const_node_ptr node)
	{}

};

class debug_command : public wml_command {
	const_formula_ptr text_;
	void do_execute(const formula_callable& info, world& world) const {
		std::cerr << "DEBUG " << SDL_GetTicks() << ": " << text_->execute(info).as_string() << "\n";
	}
public:
	explicit debug_command(wml::const_node_ptr node)
	    : text_(formula::create_string_formula(node->attr("text")))
	{}
};

class compound_command : public wml_command {
	std::vector<const_wml_command_ptr> cmd_;
	void do_execute(const formula_callable& info, world& world) const {
		foreach(const const_wml_command_ptr& cmd, cmd_) {
			cmd->execute(info, world);
		}
	}
public:
	explicit compound_command(wml::const_node_ptr node) {
		if(!node) {
			return;
		}

		for(wml::node::const_all_child_iterator i = node->begin_children(); i != node->end_children(); ++i) {
			cmd_.push_back(create(*i));
		}
	}
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

		//for convenience, commands need not be put inside a 'then' tag
		for(wml::node::const_all_child_iterator i = node->begin_children();
		    i != node->end_children(); ++i) {
			if((*i)->name() != "then" && (*i)->name() != "else") {
				then_.push_back(wml_command::create(*i));
			}
		}

		wml::const_node_ptr else_branch = node->get_child("else");
		if(else_branch) {
			for(wml::node::const_all_child_iterator i = else_branch->begin_children();
			    i != else_branch->end_children(); ++i) {
				else_.push_back(wml_command::create(*i));
			}
		}
	}
};

class while_command : public wml_command {
	formula condition_;
	std::vector<const_wml_command_ptr> commands_;
	void do_execute(const formula_callable& info, world& world) const {
		while(condition_.execute(info).as_bool()) {
			foreach(const const_wml_command_ptr& cmd, commands_) {
				cmd->execute(info, world);
			}
		}
	}
public:
	explicit while_command(wml::const_node_ptr node) : condition_(node->attr("condition"))
	{
		for(wml::node::const_all_child_iterator i = node->begin_children();
		    i != node->end_children(); ++i) {
				commands_.push_back(wml_command::create(*i));
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
		map_formula_callable_ptr callable(new map_formula_callable(&info));
		for(int n = 0; n != objects.num_elements(); ++n) {
			variant obj = objects[n];
			callable->add("object", obj);
			for(std::map<std::string,const_formula_ptr>::const_iterator i = modify_.begin(); i != modify_.end(); ++i) {
				obj.mutable_callable()->mutate_value(i->first, i->second->execute(*callable));
			}
		}
	}
public:
	explicit modify_objects_command(wml::const_node_ptr node)
	   : object_finder_(node->attr("objects").empty() ? node->attr("object") : node->attr("objects"))
	{
		for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
			if(i->first != "objects") {
				modify_[i->first].reset(new formula(i->second));
			}
		}
	}
};

class shop_command : public wml_command {
	std::string items_;
	formula cost_;
	formula pc_;
	void do_execute(const formula_callable& info, world& world) const {
		game_dialogs::shop_dialog(*pc_.execute(info).convert_to<party>(), cost_.execute(info).as_int(), items_).show_modal();
	}
public:
	explicit shop_command(wml::const_node_ptr node)
	   : items_(node->attr("items")), cost_(wml::get_str(node, "cost", "100")), pc_(wml::get_str(node, "pc", "pc"))
	{}
};

class battle_command : public wml_command {
	formula loc_;
	formula pc_chars_, npc_chars_;
	formula pc_party_, npc_party_;
	const_wml_command_ptr onvictory_, ondefeat_;
	void do_execute(const formula_callable& info, world& world) const {
		const hex::location_ptr battle_loc(loc_.execute(info).convert_to<hex::location>());
		variant pc_chars = pc_chars_.execute(info);
		variant npc_chars = npc_chars_.execute(info);

		std::vector<character_ptr> pc_chars_vector, npc_chars_vector;
		for(int n = 0; n != pc_chars.num_elements(); ++n) {
			character* c = pc_chars[n].try_convert<character>();
			if(c) {
				pc_chars_vector.push_back(c);
			}
		}

		for(int n = 0; n != npc_chars.num_elements(); ++n) {
			character* c = npc_chars[n].try_convert<character>();
			if(c) {
				npc_chars_vector.push_back(c);
			}
		}
		
		party_ptr pc_party(pc_party_.execute(info).convert_to<party>());
		party_ptr npc_party(npc_party_.execute(info).convert_to<party>());

		const bool victory = play_battle(pc_party, npc_party, pc_chars_vector, npc_chars_vector, *battle_loc);

		if(victory && onvictory_) {
			onvictory_->execute(info, world);
		}

		if(!victory && ondefeat_) {
			ondefeat_->execute(info, world);
		}
	}
public:
	explicit battle_command(wml::const_node_ptr node)
		: loc_(wml::get_str(node, "loc", "npc.loc")),
		  pc_chars_(wml::get_str(node, "pc_chars", "pc.members")),
		  npc_chars_(wml::get_str(node, "npc_chars", "npc.members")),
		  pc_party_(wml::get_str(node, "pc_party", "pc")),
		  npc_party_(wml::get_str(node, "npc_party", "npc")),
		  onvictory_(new compound_command(node->get_child("onvictory"))),
		  ondefeat_(new compound_command(node->get_child("ondefeat")))
	{
	}
};

class party_chat_command : public wml_command {
	const_formula_ptr text_;
	formula speaker_, delay_;
	void do_execute(const formula_callable& info, world& world) const {
		const character* c = speaker_.execute(info).try_convert<character>();
		if(!c) {
			return;
		}

		const int delay = delay_.execute(info).as_int();
		variant var = text_->execute(info);
		const SDL_Color col = {0xFF, 0xFF, 0xFF, 0xFF};
		std::cerr << "ADD CHAT LABEL: '" << var.as_string() << "'\n";
		world.add_chat_label(gui::label::create(var.as_string(), col), c, delay);
	}
public:
	explicit party_chat_command(wml::const_node_ptr node)
	   : text_(formula::create_string_formula(wml::get_str(node, "text"))),
	     speaker_(wml::get_str(node, "speaker")),
		 delay_(wml::get_str(node, "delay", "0"))
	{}
};

class dialog_command : public wml_command {
	formula pc_formula_, npc_formula_;
	const_formula_ptr condition_;
	const_formula_ptr text_;
	std::vector<std::string> options_;
	std::vector<const_formula_ptr> option_conditions_;
	std::vector<std::vector<const_wml_command_ptr> > consequences_;
	void do_execute(const formula_callable& info, world& world) const {
		if(condition_) {
			variant res = condition_->execute(info);
			if(!res.as_bool()) {
				return;
			}
		}

		std::cerr << "pc: " << pc_formula_.str() << "\n";
		const variant pc = pc_formula_.execute(info);
		std::cerr << "npc: " << npc_formula_.str() << "\n";
		const variant npc = npc_formula_.execute(info);
		std::cerr << "done\n";

		const character* pc_char = NULL;
		const character* npc_char = NULL;
		const party* pc_party = pc.try_convert<const party>();
		const party* npc_party = npc.try_convert<const party>();

		std::string pc_image, npc_image;

		if(pc_party) {
			pc_char = pc_party->begin_members()->get();
		} else {
			pc_char = pc.try_convert<const character>();
		}

		if(pc_char) {
			pc_image = pc_char->portrait();
		}

		if(npc_party) {
			npc_char = npc_party->begin_members()->get();
		} else {
			npc_char = npc.try_convert<const character>();
		}

		if(npc_char) {
			npc_image = npc_char->portrait();
		}

		std::map<int,int> options_map_;
		std::vector<std::string> options;
		for(int n = 0; n != options_.size(); ++n) {
			if(!option_conditions_[n] || option_conditions_[n]->execute(info).as_bool()) {
				options_map_[options.size()] = n;
				options.push_back(options_[n]);
			}
		}

		gui::message_dialog dialog(world, pc_image, npc_image, text_->execute(info).as_string(),
		                           options.empty() ? NULL : &options,
								   false);
		dialog.show_modal();
		if(!options.empty()) {
			const int option = options_map_[dialog.selected()];
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
		 condition_(formula::create_optional_formula(wml::get_str(node, "condition"))),
		 text_(formula::create_string_formula(wml::get_str(node, "text")))
	{
		wml::node::const_child_range options = node->get_child_range("option");
		while(options.first != options.second) {
			const wml::const_node_ptr op = options.first->second;
			const std::string cond = wml::get_str(op, "condition");
			if(cond.empty()) {
				option_conditions_.push_back(const_formula_ptr());
			} else {
				option_conditions_.push_back(const_formula_ptr(new formula(cond)));
			}
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

namespace {

void formula_substitute_wml(wml::node_ptr node, const formula_callable& info) {
	for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
		node->set_attr(i->first, formula::evaluate(formula::create_string_formula(i->second), info).as_string());
	}

	for(wml::node::all_child_iterator i = node->begin_children(); i != node->end_children(); ++i) {
		formula_substitute_wml(*i, info);
	}
}

}

class party_command : public wml_command
{
	wml::const_node_ptr node_;
	void do_execute(const formula_callable& info, world& world) const {
		wml::node_ptr node(wml::deep_copy(node_));
		formula_substitute_wml(node, info);
		world.add_party(party::create_party(node, world));
	}
public:
	explicit party_command(wml::const_node_ptr node) : node_(node)
	{}
};

class quit_command : public wml_command
{
	void do_execute(const formula_callable& info, world& world) const {
		world.quit();
	}
public:
	explicit quit_command(wml::const_node_ptr node)
	{}
};

class character_status_dialog_command : public wml_command
{
	formula character_;
	void do_execute(const formula_callable& info, world& world) const {
		variant var = character_.execute(info);
		character_ptr ch(dynamic_cast<character*>(var.mutable_callable()));
		if(ch) {
			game_dialogs::character_status_dialog(ch, party_ptr()).show_modal();
		} else {
			std::cerr << "ERROR: formula '" << character_.str() << "' did not return a character\n";
		}
	}
public:
	explicit character_status_dialog_command(wml::const_node_ptr node)
	    : character_(node->attr("character"))
	{}
};

class fire_event_command : public wml_command
{
	std::string event_;
	std::map<std::string, const_formula_ptr> params_;
	void do_execute(const formula_callable& info, world& world) const {
		map_formula_callable_ptr callable(new map_formula_callable(&info));
		for(std::map<std::string, const_formula_ptr>::const_iterator i = params_.begin(); i != params_.end(); ++i) {
			callable->add(i->first, i->second->execute(info));
		}

		world.fire_event(event_, *callable);
	}

public:
	explicit fire_event_command(wml::const_node_ptr node)
	    : event_(node->attr("event"))
	{
		for(wml::node::const_attr_iterator i = node->begin_attr(); i != node->end_attr(); ++i) {
			if(i->first != "event") {
				params_[i->first].reset(new formula(i->second));
			}
		}
	}
};

class null_command : public wml_command
{
	void do_execute(const formula_callable& info, world& world) const {
	}
};

}

const_wml_command_ptr wml_command::create(wml::const_node_ptr node)
{
	static const_wml_command_ptr empty_command(new null_command);
	if(!node) {
		return empty_command;
	}

#define DEFINE_COMMAND(cmd) \
	if(node->name() == #cmd) { \
		return const_wml_command_ptr(new cmd##_command(node)); \
	}

#define DEFINE_COMMAND_SYNONYM(cmd, synonym) \
	if(node->name() == #synonym) { \
		return const_wml_command_ptr(new cmd##_command(node)); \
	}
	

	try {
	DEFINE_COMMAND(debug);
	DEFINE_COMMAND(debug_console);
	DEFINE_COMMAND(destroy_party);
	DEFINE_COMMAND(if);
	DEFINE_COMMAND(while);
	DEFINE_COMMAND(scripted_moves);
	DEFINE_COMMAND(execute_script);
	DEFINE_COMMAND(modify_objects);
	DEFINE_COMMAND_SYNONYM(modify_objects, set);
	DEFINE_COMMAND(battle);
	DEFINE_COMMAND(party_chat);
	DEFINE_COMMAND(dialog);
	DEFINE_COMMAND(shop);
	DEFINE_COMMAND(party);
	DEFINE_COMMAND(quit);
	DEFINE_COMMAND(character_status_dialog);
	DEFINE_COMMAND(fire_event);
	} catch(...) {
		std::string str;
		wml::write(node, str);
		std::cerr << "ERROR in command:\n" << str << "\n";
		throw;
	}

	return empty_command;
}

}
