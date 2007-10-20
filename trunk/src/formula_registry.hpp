#ifndef FORMULA_REGISTRY_HPP_INCLUDED
#define FORMULA_REGISTRY_HPP_INCLUDED

#include <string>

#include "formula_fwd.hpp"
#include "wml_node_fwd.hpp"

namespace formula_registry
{
typedef game_logic::const_formula_ptr ptr;
void load(const wml::const_node_ptr& node);
const ptr& get_stat_calculation(const std::string& key);
const ptr& get_strength_penalty(const std::string& key);
const ptr& get_fatigue_penalty(const std::string& key);
const ptr& get_height_advantage(const std::string& key);

const ptr& get_track_formula();

}

#endif
