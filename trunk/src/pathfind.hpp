#ifndef PATHFIND_HPP_INCLUDED
#define PATHFIND_HPP_INCLUDED

#include <vector>

#include "tile_logic.hpp"

namespace hex
{

class path_cost_calculator {
public:
	virtual ~path_cost_calculator();
	virtual int movement_cost(const location& a, const location& b) const;
	virtual int estimated_cost(const location& a, const location& b) const;
	virtual bool allowed_to_move(const location& a) const;
};

int find_path(const location& src, const location& dst, const path_cost_calculator& calc, std::vector<location>* result, int max_cost=10000, bool adjacent_only=false);

}

#endif
