#include <algorithm>

#include "foreach.hpp"
#include "formatter.hpp"
#include "party.hpp"
#include "tracks.hpp"
#include "wml_node.hpp"
#include "wml_utils.hpp"

namespace game_logic
{

namespace {
bool track_invisible(const tracks::info& i) {
	return i.visibility <= 0;
}
}

void tracks::read(const wml::const_node_ptr& node)
{
	tracks_.clear();
	for(wml::node::const_child_range r = node->get_child_range("track");
	    r.first != r.second; ++r.first) {
		wml::const_node_ptr node = r.first->second;
		info i;
		i.visibility = wml::get_int(node, "visibility");
		i.dir = static_cast<hex::DIRECTION>(wml::get_int(node, "direction"));
		i.party_id = wml::get_int(node, "party_id");
		i.time = game_time(node);
		i.last_update = i.time;
		hex::location loc(wml::get_int(node,"x"),wml::get_int(node,"y"));
		tracks_[loc].push_back(i);
	}
}

wml::node_ptr tracks::write() const
{
	wml::node_ptr res(new wml::node("tracks"));
	for(std::map<hex::location,tracks_list>::iterator i = tracks_.begin();
	    i != tracks_.end(); ++i) {
		tracks_list& list = i->second;
		foreach(info& i, list) {
			const int diff = i.time - i.last_update;
			i.last_update = i.time;
			i.visibility -= diff;
		}
		list.erase(std::remove_if(list.begin(),list.end(),track_invisible), list.end());
		foreach(const info& t, list) {
			const wml::node_ptr node(hex::write_location("track", i->first));
			node->set_attr("visibility", formatter() << t.visibility);
			node->set_attr("direction", formatter() << static_cast<int>(t.dir));
			node->set_attr("party_id", formatter() << t.party_id);
			node->set_attr("time", formatter() << t.time.since_epoch());
			res->add_child(node);
		}

	}

	return res;
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
