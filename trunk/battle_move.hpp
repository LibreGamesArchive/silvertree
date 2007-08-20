#ifndef BATTLE_MOVE_HPP_INCLUDED
#define BATTLE_MOVE_HPP_INCLUDED

#include <map>

#include "battle_character_fwd.hpp"
#include "battle_modification.hpp"
#include "battle_move_fwd.hpp"
#include "formula_fwd.hpp"
#include "wml_node.hpp"

namespace game_logic
{

class battle_move {
public:
	static const_battle_move_ptr standard_move();
	static const_battle_move_ptr standard_attack();
	static const_battle_move_ptr standard_pass();
	explicit battle_move(wml::const_node_ptr node);
	const std::string& name() const { return name_; }
	const std::string& category() const { return category_; }
	int max_moves() const { return moves_; }
	int min_moves() const { return min_moves_; }
	bool can_attack() const { return can_attack_; }
	bool must_attack() const { return must_attack_; }
	int get_stat(const std::string& stat, const battle_character& c) const;

	const battle_modification_ptr& mod() const { return mod_; }
private:
	battle_move() {}
	std::string name_;
	std::string category_;
	int moves_;
	int min_moves_;
	bool can_attack_;
	bool must_attack_;
	std::map<std::string,const_formula_ptr> stats_;
	battle_modification_ptr mod_;
};

}

#endif
