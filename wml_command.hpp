#ifndef WML_COMMAND_HPP_INCLUDED
#define WML_COMMAND_HPP_INCLUDED

#include "wml_command_fwd.hpp"
#include "wml_node_fwd.hpp"

namespace game_logic
{

class formula_callable;
class world;

class wml_command
{
public:
	static const_wml_command_ptr create(wml::const_node_ptr node);

	void execute(const formula_callable& info, world& world) const { do_execute(info, world); }

private:
	virtual void do_execute(const formula_callable& info, world& world) const = 0;
};

}

#endif
