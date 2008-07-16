#ifndef VARIABLES_CALLABLE_HPP_INCLUDED
#define VARIABLES_CALLABLE_HPP_INCLUDED

#include <map>
#include <string>

#include "formula_callable.hpp"
#include "variant.hpp"

namespace game_logic {
typedef std::map<std::string, variant> variable_map;
class variables_callable : public formula_callable
{
public:
	explicit variables_callable(variable_map& var) : var_(var) {
	}
private:
	void get_inputs(std::vector<formula_input>* inputs) const {
		for(variable_map::const_iterator i = var_.begin(); i != var_.end(); ++i) {
			inputs->push_back(formula_input(i->first, FORMULA_READ_WRITE));
		}
	}

	variant get_value(const std::string& key) const {
		variable_map::const_iterator i = var_.find(key);
		if(i != var_.end()) {
			return i->second;
		} else {
			return variant();
		}
	}

	void set_value(const std::string& key, const variant& value) {
		if(key == "tmp") {
			return;
		}
		var_[key] = value;
	}

	variable_map& var_;
};

}

#endif
