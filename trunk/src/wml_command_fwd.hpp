#ifndef WML_COMMAND_FWD_HPP_INCLUDED
#define WML_COMMAND_FWD_HPP_INCLUDED

#include <boost/shared_ptr.hpp>

namespace game_logic {

class wml_command;
typedef boost::shared_ptr<wml_command> wml_command_ptr;
typedef boost::shared_ptr<const wml_command> const_wml_command_ptr;

}

#endif
