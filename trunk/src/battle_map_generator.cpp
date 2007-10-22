
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <iostream>
#include <stdlib.h>

#include "base_terrain.hpp"
#include "battle_map_generator.hpp"
#include "foreach.hpp"
#include "gamemap.hpp"
#include "terrain_feature.hpp"
#include "tile.hpp"
#include "tile_logic.hpp"
#include "zoom_map_generator.hpp"

namespace game_logic
{

namespace {

void calculate_route(const hex::location& src, const hex::location& dst,
                     std::vector<hex::location>* route) {
	hex::location loc = src;
	while(loc != dst) {
		hex::location adj[6];
		hex::get_adjacent_tiles(loc,adj);
		std::vector<hex::location> options;
		int best = hex::distance_between(loc,dst)-1;
		foreach(const hex::location& a, adj) {
			if(a == dst) {
				route->push_back(dst);
				return;
			} else {
				const int distance = hex::distance_between(a,dst);
				if(distance == best) {
					options.push_back(a);
				} else if(best == -1 || distance < best) {
					options.clear();
					options.push_back(a);
					best = distance;
				}
			}
		}

		if(options.empty()) {
			return;
		}

		loc = options[rand()%options.size()];
		route->push_back(loc);
	}
}

}

boost::shared_ptr<hex::gamemap> generate_battle_map(
           const hex::gamemap& world_map, const hex::location& loc)
{
	return hex::generate_zoom_map(world_map, hex::location(loc.x()-2,loc.y()-2), hex::location(5,5), hex::location(50,50), 1.0, 1.0);
	hex::location adj[6];
	get_adjacent_tiles(loc,adj);

	hex::const_base_terrain_ptr adj_terrain[6];
	const hex::tile* adj_tiles[6];
	for(int n = 0; n != 6; ++n) {
		if(world_map.is_loc_on_map(adj[n])) {
			adj_tiles[n] = &world_map.get_tile(adj[n]);
			adj_terrain[n] = adj_tiles[n]->terrain();
		} else {
			adj_tiles[n] = NULL;
		}
	}

	const hex::location dim(10,10);
	const hex::tile& center_tile = world_map.get_tile(loc);
	hex::const_base_terrain_ptr center_terrain = center_tile.terrain();
	hex::const_base_terrain_ptr path_terrain;
	int path_directions[] = {-1,-1};

	std::vector<hex::location> route;

	std::cerr << "style = '" << center_terrain->battle_style() << "'; count = " << std::count(adj_terrain,adj_terrain+6,center_terrain) << "\n";
	if(center_terrain->battle_style() == "path" &&
	   std::count(adj_terrain,adj_terrain+6,center_terrain) == 2) {
		std::cerr << "on path\n";
		int best = 0;
		hex::const_base_terrain_ptr best_terrain;
		int dir_index = 0;
		for(int n = 0; n != 6; ++n) {
			if(adj_terrain[n] == center_terrain) {
				path_directions[dir_index++] = n;
			}

			const int count = std::count(adj_terrain,adj_terrain+6,
			                             adj_terrain[n]);
			if(count > best && adj_terrain[n] &&
			   adj_terrain[n]->battle_style() != "path") {
				best_terrain = adj_terrain[n];
				best = count;
			}
		}

		if(best_terrain) {
			std::cerr << "found best\n";
			path_terrain = center_terrain;
			std::replace(adj_terrain, adj_terrain+6,
			             center_terrain, best_terrain);
			center_terrain = best_terrain;

			for(int n = 0; n != 2; ++n) {
				hex::location src;
				switch(path_directions[n]) {
				case 0:
					src = hex::location(5,0);
					break;
				case 1:
					src = hex::location(10,0);
					break;
				case 2:
					src = hex::location(10,10);
					break;
				case 3:
					src = hex::location(5,10);
					break;
				case 4:
					src = hex::location(0,10);
					break;
				case 5:
					src = hex::location(0,0);
					break;
				}
				calculate_route(src,hex::location(5,5),&route);
			}
		}
	}

	std::vector<hex::tile> tiles;
	for(int y = 0; y != dim.y(); ++y) {
		for(int x = 0; x != dim.x(); ++x) {
			const hex::tile* t = &center_tile;
			hex::const_base_terrain_ptr terrain = center_terrain;
			if(y < 2 && adj_tiles[0] && (rand()%100) < 50) {
				t = adj_tiles[0];
				terrain = adj_terrain[0];
			}

			if(y > 8 && adj_tiles[3] && (rand()%100) < 50) {
				t = adj_tiles[3];
				terrain = adj_terrain[3];
			}

			if(y < 4 && x > 6 && adj_tiles[1] && (rand()%100) < 50) {
				t = adj_tiles[1];
				terrain = adj_terrain[1];
			}

			if(y > 6 && x > 6 && adj_tiles[2] && (rand()%100) < 50) {
				t = adj_tiles[2];
				terrain = adj_terrain[2];
			}

			if(y > 6 && x < 4 && adj_tiles[4] && (rand()%100) < 50) {
				t = adj_tiles[4];
				terrain = adj_terrain[4];
			}

			if(y < 4 && x < 4 && adj_tiles[5] && (rand()%100) < 50) {
				t = adj_tiles[5];
				terrain = adj_terrain[5];
			}

			hex::location loc(x,y);

			if(path_terrain) {
				std::cerr << "has path terrain\n";
				const int distance = 2 + rand()%2;
				foreach(const hex::location& r, route) {
					if(hex::distance_between(r,loc) < distance) {
						terrain = path_terrain;
						break;
					}
				}
			}

			const int height = int(t->height_at_point(x*0.1,y*0.1));
			tiles.push_back(hex::tile(loc,height*3,terrain,
			                          t->feature()));
		}
	}

	return boost::shared_ptr<hex::gamemap>(
	                new hex::gamemap(tiles,dim));
}

}
