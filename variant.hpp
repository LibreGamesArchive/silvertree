#ifndef VARIANT_HPP_INCLUDED
#define VARIANT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

namespace game_logic {
class formula_callable;
}

struct variant_list;
struct variant_string;

class variant {
public:
	explicit variant(int n=0);
	explicit variant(const game_logic::formula_callable* callable);
	explicit variant(std::vector<variant>* array);
	explicit variant(const std::string& str);

	variant(const variant& v);
	const variant& operator=(const variant& v);

	const variant& operator[](size_t n) const;
	size_t num_elements() const;

	bool is_int() const { return type_ == TYPE_INT; }
	int as_int() const { must_be(TYPE_INT); return int_value_; }
	bool as_bool() const { return int_value_ != 0; }

	const std::string& as_string() const;

	bool is_callable() const { return type_ == TYPE_CALLABLE; }
	const game_logic::formula_callable* as_callable() const {
		must_be(TYPE_CALLABLE); return callable_; }
	game_logic::formula_callable* mutable_callable() {
		must_be(TYPE_CALLABLE); return mutable_callable_; }

	variant operator+(const variant&) const;
	variant operator-(const variant&) const;
	variant operator*(const variant&) const;
	variant operator/(const variant&) const;
	variant operator^(const variant&) const;
	variant operator-() const;

	bool operator==(const variant&) const;
	bool operator!=(const variant&) const;
	bool operator<(const variant&) const;
	bool operator>(const variant&) const;
	bool operator<=(const variant&) const;
	bool operator>=(const variant&) const;

	void serialize_to_string(std::string& str) const;
	void serialize_from_string(const std::string& str);

	std::string string_cast() const;

private:

	enum TYPE { TYPE_INT, TYPE_CALLABLE, TYPE_LIST, TYPE_STRING };
	void must_be(TYPE t) const;
	TYPE type_;
	union {
		int int_value_;
		const game_logic::formula_callable* callable_;
		game_logic::formula_callable* mutable_callable_;
		variant_list* list_;
		variant_string* string_;
	};

	void increment_refcount();
	void release();
};

struct type_error {
	explicit type_error(const std::string& str) {}
};

#endif
