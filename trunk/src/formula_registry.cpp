#include "formula.hpp"
#include "formula_registry.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace formula_registry
{
using namespace game_logic;

namespace {
typedef std::map<std::string,ptr> formula_map;
formula_map stat_calculation;
formula_map strength_penalty;
formula_map fatigue_penalty;
formula_map height_advantage;

ptr track_formula;

const ptr NullFormula;
}

void load(const wml::const_node_ptr& node)
{
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		stat_calculation[i->first] = ptr(new formula(i->second));
	}

	wml::const_node_ptr penalties = node->get_child("ideal_strength_penalties");
	if(penalties) {
		for(wml::node::const_attr_iterator i = penalties->begin_attr();
		    i != penalties->end_attr(); ++i) {
			strength_penalty[i->first] = ptr(new formula(i->second));
		}
	}

	penalties = node->get_child("fatigue_penalties");
	if(penalties) {
		for(wml::node::const_attr_iterator i = penalties->begin_attr();
		    i != penalties->end_attr(); ++i) {
			fatigue_penalty[i->first] = ptr(new formula(i->second));
		}
	}

	wml::const_node_ptr height = node->get_child("height_advantage");
	if(height) {
		for(wml::node::const_attr_iterator i = height->begin_attr();
		    i != height->end_attr(); ++i) {
			height_advantage[i->first] = ptr(new formula(i->second));
		}
	}

	wml::const_node_ptr rules = node->get_child("rules");
	if(rules) {
		if(rules->has_attr("track")) {
			track_formula.reset(new formula((*rules)["track"]));
		}
	}
}

const ptr& get_track_formula()
{
	return track_formula;
}

#define FORMULA_ACCESSOR(name) \
const ptr& get_##name(const std::string& key) { \
	const formula_map::const_iterator itor = name.find(key); \
	if(itor != name.end()) { \
		return itor->second; \
	} else { \
		return NullFormula; \
	} \
}

FORMULA_ACCESSOR(stat_calculation);
FORMULA_ACCESSOR(strength_penalty);
FORMULA_ACCESSOR(fatigue_penalty);
FORMULA_ACCESSOR(height_advantage);

}
