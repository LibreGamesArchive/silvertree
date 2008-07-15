#include "battle_character.hpp"
#include "battle_modification.hpp"
#include "character.hpp"
#include "floating_label.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "text.hpp"
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

	void get_inputs(std::vector<formula_input>* inputs) const {
		inputs->push_back(formula_input("target", FORMULA_READ_ONLY));
		inputs->push_back(formula_input("caster", FORMULA_READ_ONLY));
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
		} else if(i->first == "range") {
			range_.reset(new formula(i->second));
		} else if(i->first == "radius") {
			radius_.reset(new formula(i->second));
		} else {
			mods_[i->first].reset(new formula(i->second));
		}
	}
}

void battle_modification::apply(battle_character& src,
                                battle_character& target,
								int current_time) const
{
	mod_callable callable(target.get_character(), src.get_character());
	int duration = -1;
	if(duration_) {
		duration = duration_->execute(callable).as_int();
	}

	const SDL_Color red = {0xFF,0x0,0x0,0xFF};
	const SDL_Color blue = {0x0,0x0,0xFF,0xFF};
	const GLfloat move[3] = {0.0,0.0,0.01};
	GLfloat pos[3];
	GLfloat rotate;
	target.get_pos(pos,&rotate);

	for(std::map<std::string,formula_ptr>::const_iterator i = mods_.begin();
	    i != mods_.end(); ++i) {
		const int value = i->second->execute(callable).as_int();

		if(i->first == "inflict_damage") {
                    text::renderer& renderer = text::renderer::instance();
                    target.get_character().take_damage(value);
                    graphics::floating_label::add(renderer.render(formatter() << value, 20, 
                                                                   value <= 0 ? blue : red)->as_texture(), 
                                                  pos, move, 1000);
		} else {
			target.add_modification(i->first, current_time+duration, value);
		}
	}
}

int battle_modification::range() const
{
	return range_ ? range_->execute().as_int() : 0;
}

int battle_modification::radius() const
{
	return radius_ ? radius_->execute().as_int() : 0;
}

}
