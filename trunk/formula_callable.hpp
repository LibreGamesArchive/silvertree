#ifndef FORMULA_CALLABLE_HPP_INCLUDED
#define FORMULA_CALLABLE_HPP_INCLUDED

#include <map>
#include <string>

#include "reference_counted_object.hpp"
#include "variant.hpp"

namespace game_logic
{

enum FORMULA_ACCESS_TYPE { FORMULA_READ_ONLY, FORMULA_WRITE_ONLY, FORMULA_READ_WRITE };
struct formula_input {
	std::string name;
	FORMULA_ACCESS_TYPE access;
	explicit formula_input(const std::string& name, FORMULA_ACCESS_TYPE access=FORMULA_READ_WRITE)
			: name(name), access(access)
	{}
};

//interface for objects that can have formulae run on them
class formula_callable : public reference_counted_object {
public:
	variant query_value(const std::string& key) const {
		return get_value(key);
	}

	void mutate_value(const std::string& key, const variant& value) {
		set_value(key, value);
	}

	std::vector<formula_input> inputs() const {
		std::vector<formula_input> res;
		get_inputs(&res);
		return res;
	}

	virtual void get_inputs(std::vector<formula_input>* inputs) const = 0;
protected:
	virtual ~formula_callable() {}

	virtual void set_value(const std::string& key, const variant& value);
private:
	virtual variant get_value(const std::string& key) const = 0;
};

class map_formula_callable : public formula_callable {
public:
	explicit map_formula_callable(const formula_callable* fallback=NULL);
	map_formula_callable& add(const std::string& key, const variant& value);
private:
	variant get_value(const std::string& key) const;
	void get_inputs(std::vector<formula_input>* inputs) const;
	std::map<std::string,variant> values_;
	const formula_callable* fallback_;
};

typedef boost::intrusive_ptr<map_formula_callable> map_formula_callable_ptr;
typedef boost::intrusive_ptr<const map_formula_callable> const_map_formula_callable_ptr;

}

#endif
