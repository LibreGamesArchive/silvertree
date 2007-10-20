#ifndef CHARACTER_GENERATOR_HPP_INCLUDED
#define CHARACTER_GENERATOR_HPP_INCLUDED

#include <string>

#include "character_fwd.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class character_generator
{
public:
	static void initialize(wml::const_node_ptr node);
	static const character_generator& get(const std::string& name="");
	static std::vector<std::string> generator_list();

	character_generator() {}

	void generate(character& c, wml::const_node_ptr node) const;

private:
	explicit character_generator(wml::const_node_ptr node);

	wml::const_node_ptr node_;
};

}

#endif
