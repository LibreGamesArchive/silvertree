#ifndef LOCATION_FWD_HPP_INCLUDED
#define LOCATION_FWD_HPP_INCLUDED

#include <boost/intrusive_ptr.hpp>

namespace hex {

class location;
typedef boost::intrusive_ptr<location> location_ptr;
typedef boost::intrusive_ptr<const location> const_location_ptr;

}

#endif
