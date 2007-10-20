#ifndef PARTY_FWD_HPP_INCLUDED
#define PARTY_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

namespace game_logic {

class party;
typedef boost::intrusive_ptr<party> party_ptr;
typedef boost::intrusive_ptr<const party> const_party_ptr;

}

#endif
