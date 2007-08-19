#include "character.hpp"
#include "character_generator.hpp"
#include "foreach.hpp"
#include "formula.hpp"
#include "item.hpp"
#include "skill.hpp"
#include "string_utils.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

#include <iostream>

namespace game_logic
{

namespace {
std::map<std::string,character_generator> gen_map;
}

void character_generator::initialize(wml::const_node_ptr node)
{
	if(!node) {
		return;
	}

	for(wml::node::const_child_range r = node->get_child_range("character_generator"); r.first != r.second; ++r.first) {
		wml::const_node_ptr n = r.first->second;
		gen_map[(*n)["id"]] = character_generator(n);
	}
}

const character_generator& character_generator::get(const std::string& name)
{
	if(name == "") {
		static character_generator empty;
		return empty;
	}

	std::map<std::string,character_generator>::const_iterator i = gen_map.find(name);
	if(i == gen_map.end()) {
		std::cerr << "could not find char generator '" << name << "'\n";
		static character_generator empty;
		return empty;
	}

	return i->second;
}

std::vector<std::string> character_generator::generator_list()
{
	std::vector<std::string> res;
	for(std::map<std::string,character_generator>::const_iterator i =
	    gen_map.begin(); i != gen_map.end(); ++i) {
		res.push_back(i->first);
	}

	return res;
}

character_generator::character_generator(wml::const_node_ptr node)
  : node_(node)
{}

namespace {
template<typename T>
void read_attr(T* res, wml::const_node_ptr n1, wml::const_node_ptr n2,
               const std::string& attr) {
	if(n1 && n1->has_attr(attr)) {
		*res = wml::get_attr<T>(n1,attr);
	} else if(n2 && n2->has_attr(attr)) {
		*res = wml::get_attr<T>(n2,attr);
	}
}

wml::const_node_ptr get_child(wml::const_node_ptr n1, wml::const_node_ptr n2,
                              const std::string& name)
{
	if(n1 && n1->get_child(name)) {
		return n1->get_child(name);
	}

	if(n2 && n2->get_child(name)) {
		return n2->get_child(name);
	}

	return wml::const_node_ptr();
}
}

void character_generator::generate(character& c, wml::const_node_ptr node) const
{
#define READ_ATTR(attr,def) c.attr##_ = def; \
		                    read_attr(&c.attr##_, node, node_, #attr)
	READ_ATTR(description, "");
	READ_ATTR(fatigue, 0);
	READ_ATTR(level, 1);
	READ_ATTR(xp, 0);
	READ_ATTR(image, "");
	READ_ATTR(portrait, "");
	READ_ATTR(improvement_points, 0);
	READ_ATTR(spent_skill_points, 0);

	std::string align;
	read_attr(&align, node, node_, "alignment");
	c.alignment_ = character::NEUTRAL;
	if(align == "chaotic") {
		c.alignment_ = character::CHAOTIC;
	} else if(align == "lawful") {
		c.alignment_ = character::LAWFUL;
	}

	wml::const_node_ptr costs = get_child(node, node_, "movement_costs");
	if(costs) {
		for(wml::node::const_attr_iterator i = costs->begin_attr();
		    i != costs->end_attr(); ++i) {
			c.move_cost_map_[i->first] = wml::get_attr<int>(costs,i->first,-1);
		}
	}

	wml::const_node_ptr attr = get_child(node, node_, "attributes");
	if(attr) {
		for(wml::node::const_attr_iterator i = attr->begin_attr();
		    i != attr->end_attr(); ++i) {
			c.attributes_[i->first] = formula(i->second).execute().as_int();
		}
	}

	wml::node::const_child_range equip = node->get_child_range("equipment");
	while(equip.first != equip.second) {
		const wml::const_node_ptr eq = equip.first->second;
		const item_ptr item = item::create_item(eq);
		if(item) {
			c.equipment_.push_back(item);
		}

		++equip.first;
	}

	std::string skills_str;
	read_attr(&skills_str, node, node_, "skills");
	const std::vector<std::string> skills = util::split(skills_str);
	foreach(const std::string& skill, skills) {
		c.skills_.push_back(skill::get_skill(skill));
	}
	
	READ_ATTR(hitpoints, c.max_hitpoints());
	c.calculate_moves();
}

}
