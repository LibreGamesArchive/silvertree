#include "battle_character.hpp"
#include "battle_modification.hpp"
#include "character.hpp"
#include "formula.hpp"
#include "wml_node.hpp"

namespace game_logic
{

namespace {

class mod_callable : public formula_callable
{
	const character& target_;
	const character& caster_;
	variant get_value(const std::string& stat) const {
		if(stat == "target") {
			return variant(&target_);
		} else if(stat == "caster") {
			return variant(&caster_);
		} else {
			return caster_.query_value(stat);
		}
	}
public:
	mod_callable(const character& target, const character& caster)
	  : target_(target), caster_(caster)
	{
	}
};

}

battle_modification::battle_modification(wml::const_node_ptr node)
  : target_(TARGET_SELF)
{
	for(wml::node::const_attr_iterator i = node->begin_attr();
	    i != node->end_attr(); ++i) {
		if(i->first == "target") {
			if(i->second == "self") {
				target_ = TARGET_SELF;
			} else if(i->second == "enemy") {
				target_ = TARGET_ENEMY;
			} else if(i->second == "friend") {
				target_ = TARGET_FRIEND;
			} else if(i->second == "all") {
				target_ = TARGET_ALL;
			}
		} else if(i->first == "duration") {
			duration_.reset(new formula(i->second));
		} else {
			mods_[i->first].reset(new formula(i->second));
		}
	}
}

void battle_modification::apply(battle_character& src,
                                battle_character& target,
								int current_time)
{
	mod_callable callable(target.get_character(), src.get_character());
	int duration = -1;
	if(duration_) {
		duration = duration_->execute(callable).as_int();
	}

	for(std::map<std::string,formula_ptr>::iterator i = mods_.begin();
	    i != mods_.end(); ++i) {
		const int value = i->second->execute(callable).as_int();
		target.add_modification(i->first, current_time+duration, value);
	}
}

}
