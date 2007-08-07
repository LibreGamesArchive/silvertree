#include <algorithm>

#include "foreach.hpp"
#include "party.hpp"
#include "tracks.hpp"

namespace game_logic
{

namespace {
bool track_invisible(const tracks::info& i) {
	return i.visibility <= 0;
}
}

void tracks::add_tracks(const hex::location& loc, const party& p,
                        const game_time& t, hex::DIRECTION dir)
{
	info i;
	i.visibility = p.trackability();
	i.dir = dir;
	i.party_id = p.id();
	i.last_update = i.time = t;
	tracks_[loc].push_back(i);
}

const tracks::tracks_list& tracks::get_tracks(const hex::location& loc,
                                              const game_time& t) const
{
	static const tracks::tracks_list empty_tracks;
	std::map<hex::location,tracks_list>::iterator itor = tracks_.find(loc);
	if(itor == tracks_.end()) {
		return empty_tracks;
	}

	tracks_list& list = itor->second;
	foreach(info& i, list) {
		const int diff = t - i.last_update;
		i.last_update = t;
		i.visibility -= diff;
	}

	list.erase(std::remove_if(list.begin(),list.end(),track_invisible), list.end());
	if(list.empty()) {
		tracks_.erase(itor);
		return empty_tracks;
	}

	return list;
}

}
