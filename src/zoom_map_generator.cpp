#include <algorithm>
#include <assert.h>
#include <math.h>

#include "base_terrain.hpp"
#include "foreach.hpp"
#include "zoom_map_generator.hpp"

namespace hex {

namespace {
GLfloat distance_between_points(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
	return sqrt((x1-x2)*(x1-x2) + (y1-y2)*(y1-y2));
}

}

boost::shared_ptr<gamemap> generate_zoom_map(
    const gamemap& map, const location& top_left, const location& src_dim,
	const location& dim, double height_scale, double randomness)
{
	std::cerr << "zoom map: " << top_left.x() << ", " << top_left.y() << "\n";
	std::vector<GLfloat> heights;
	std::vector<const_base_terrain_ptr> terrains;
	std::vector<const_terrain_feature_ptr> features;
	for(int y = 0; y != dim.y(); ++y) {
		GLfloat ypos = tile::translate_y(top_left.y() + static_cast<double>(y)*(static_cast<double>(src_dim.y())/static_cast<double>(dim.y())));
		for(int x = 0; x != dim.x(); ++x) {
			GLfloat xpos = tile::translate_x(top_left.x() + static_cast<double>(x)*(static_cast<double>(src_dim.x())/static_cast<double>(dim.x())));
			GLfloat ytmp = ypos;
			GLfloat xtmp = xpos;

			const tile* src_tile = map.closest_tile(&xtmp, &ytmp, false);
			assert(src_tile);

			std::vector<tile::point> closest_points;
			closest_points.push_back(src_tile->center());
			closest_points.push_back(src_tile->corners()[0]);
			closest_points.push_back(src_tile->corners()[1]);
			for(int n = 2; n != 6; ++n) {
				GLfloat max_distance = -1.0;
				tile::point* max_point = NULL;
				for(std::vector<tile::point>::iterator p = closest_points.begin(); p != closest_points.end(); ++p) {
					double new_distance = distance_between_points(xpos, ypos, p->position.x(), p->position.y());
					if(max_point == NULL || new_distance > max_distance) {
						max_point = &*p;
						max_distance = new_distance;
					}
				}

				const tile::point& p = src_tile->corners()[n];
				const GLfloat distance = distance_between_points(xpos, ypos, p.position.x(), p.position.y());
				if(distance < max_distance) {
					*max_point = p;
				}
			}

			GLfloat distance_sum = 0.0;
			foreach(const tile::point& p, closest_points) {
				distance_sum += distance_between_points(xpos, ypos, p.position.x(), p.position.y());
			}

			GLfloat height = 0.0;
			foreach(const tile::point& p, closest_points) {
				height += p.position.z() * (1.0 - distance_between_points(xpos, ypos, p.position.x(), p.position.y())/distance_sum);
			}

			heights.push_back(height);

			// find the closest adjacent hex and see if it overlaps
			location adj[6];
			GLfloat closest_distance = -1.0;
			location closest_loc;
			get_adjacent_tiles(src_tile->loc(), adj);
			foreach(const location& a, adj) {
				if(!map.is_loc_on_map(a)) {
					continue;
				}


				const GLfloat distance = distance_between_points(xpos, ypos, tile::translate_x(a), tile::translate_y(a));
				if(closest_distance < 0.0 || distance < closest_distance) {
					closest_distance = distance;
					closest_loc = a;
				}
			}

			if(closest_distance >= 0.0) {
				assert(map.is_loc_on_map(closest_loc));
				const tile& t = map.get_tile(closest_loc);
				if(t.terrain()->overlap_priority() > src_tile->terrain()->overlap_priority()) {
					const GLfloat distance = distance_between_points(xpos, ypos, tile::translate_x(src_tile->loc()), tile::translate_y(src_tile->loc()));
					const GLfloat chance_replace = (distance/(distance + closest_distance))*randomness;
					if((rand()%100) < chance_replace*100.0) {
						src_tile = &t;
					}
				}
			}

			terrains.push_back(src_tile->terrain());
			features.push_back(src_tile->feature());
		}
	}

	std::vector<GLfloat>::const_iterator min_height = std::min_element(heights.begin(), heights.end());
	assert(min_height != heights.end());
	foreach(GLfloat& height, heights) {
		height = (height - *min_height)*height_scale;
	}

	std::vector<tile> tiles;
	
	for(int y = 0; y != dim.y(); ++y) {
		for(int x = 0; x != dim.x(); ++x) {
			const int index = y*dim.x() + x;
			assert(index < heights.size());
			tiles.push_back(tile(location(x,y), static_cast<int>(heights[index]), terrains[index], features[index]));
		}
	}

	return boost::shared_ptr<gamemap>(new gamemap(tiles, dim));
}

}
