#include <functional>
#include <iostream>
#include <queue>
#include <set>

#include "boost/shared_ptr.hpp"
#include "pathfind.hpp"

#include "SDL.h"

namespace hex
{

path_cost_calculator::~path_cost_calculator()
{
}

int path_cost_calculator::movement_cost(const location& a, const location& b) const
{
	return 100;
}

int path_cost_calculator::estimated_cost(const location& a, const location& b) const
{
	return 100*distance_between(a,b);
}

bool path_cost_calculator::allowed_to_move(const location& a) const
{
	return true;
}

namespace {

struct partial_result;
typedef boost::shared_ptr<partial_result> partial_result_ptr;
typedef boost::shared_ptr<const partial_result> const_partial_result_ptr;

struct partial_result {
	hex::location loc;
	const_partial_result_ptr prev;
	int cost_incurred;
	int estimated_cost;
};

struct partial_result_comparer : public std::binary_function<const_partial_result_ptr, const_partial_result_ptr, bool> {
	bool operator()(const const_partial_result_ptr& a, const const_partial_result_ptr& b) const {
		return a->cost_incurred + a->estimated_cost >
		       b->cost_incurred + b->estimated_cost;
	}
};

}

int find_path(const location& src, const location& dst, const path_cost_calculator& calc, std::vector<location>* result, int max_cost, bool adjacent_only)
{
	const int start_ticks = SDL_GetTicks();
	//sanity check to make sure the destination is reachable from
	//an adjacent hex
	if(!adjacent_only) {
		if(!calc.allowed_to_move(dst)) {
			return -1;
		}

		location adj[6];
		get_adjacent_tiles(dst, adj);
		bool found = false;
		for(int n = 0; n != 6; ++n) {
			if(calc.allowed_to_move(adj[n]) && calc.movement_cost(adj[n],dst) >= 0) {
				found = true;
				break;
			}
		}

		if(!found) {
			std::cerr << "infeasible_path: " << (SDL_GetTicks() - start_ticks) << "\n";
			return -1;
		}
	}

	std::set<location> closed;
	std::priority_queue<partial_result_ptr, std::vector<partial_result_ptr>, partial_result_comparer> q;
	partial_result_ptr r(new partial_result);
	r->loc = src;
	r->cost_incurred = 0;
	r->estimated_cost = calc.estimated_cost(src,dst);
	q.push(r);

	while(!q.empty()) {
		const const_partial_result_ptr r = q.top();
		q.pop();
		if(r->loc == dst) {
			for(const_partial_result_ptr p = r; p; p = p->prev) {
				result->push_back(p->loc);
			}

			std::cerr << "find_path: " << (SDL_GetTicks() - start_ticks) << "\n";
			return r->cost_incurred;
		}

		if(closed.count(r->loc)) {
			continue;
		}

		location adj[6];
		get_adjacent_tiles(r->loc, adj);
		for(int n = 0; n != 6; ++n) {
			if((!adjacent_only || adj[n] != dst) &&
			   (!calc.allowed_to_move(adj[n]) || closed.count(adj[n]))) {
				continue;
			}

			int cost = calc.movement_cost(r->loc, adj[n]);
			if(adjacent_only && adj[n] == dst) {
				cost = 0;
			}
			if(cost < 0) {
				continue;
			}
			const int estimated_cost = calc.estimated_cost(adj[n], dst);
			if(r->cost_incurred + cost + estimated_cost > max_cost) {
				continue;
			}

			partial_result_ptr p(new partial_result);
			p->loc = adj[n];
			p->prev = r;
			p->cost_incurred = r->cost_incurred + cost;
			p->estimated_cost = estimated_cost;
			q.push(p);
		}

		closed.insert(r->loc);
	}

			std::cerr << "failed_path: " << (SDL_GetTicks() - start_ticks) << "\n";
	return -1;
}

}
