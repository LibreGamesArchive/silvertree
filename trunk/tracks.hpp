#ifndef TRACKS_HPP_INCLUDED
#define TRACKS_HPP_INCLUDED

#include <map>
#include <vector>

#include "game_time.hpp"
#include "gamemap.hpp"
#include "party_fwd.hpp"
#include "tile_logic.hpp"

namespace game_logic
{

class tracks {
public:
	explicit tracks(const hex::gamemap& m) : map_(m)
	{}
	void add_tracks(const hex::location& loc, const party& p,
	                const game_time& t, hex::DIRECTION dir);

	struct info {
		int visibility;
		hex::DIRECTION dir;
		int party_id;
		game_time time;
		game_time last_update;
	};

	typedef std::vector<info> tracks_list;
	const tracks_list& get_tracks(const hex::location& loc,
	                              const game_time& t) const;
private:
	const hex::gamemap& map_;
	mutable std::map<hex::location,tracks_list> tracks_;
};

}

#endif
