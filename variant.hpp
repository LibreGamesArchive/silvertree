#ifndef VARIANT_HPP_INCLUDED
#define VARIANT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <string>

namespace game_logic {
class formula_callable;
}

struct variant_list;

class variant {
public:
	static variant_list* allocate_list(variant& v);
	static void release_list(variant_list* l);

	explicit variant(int n=0);
	explicit variant(const game_logic::formula_callable* callable);

	variant& operator[](size_t n);
	const variant& operator[](size_t n) const;
	void add_element(const variant& v);
	size_t num_elements() const;

	bool is_int() const { return type_ == TYPE_INT; }
	int as_int() const { must_be(TYPE_INT); return int_value_; }
	bool as_bool() const { return int_value_ != 0; }

	bool is_callable() const { return type_ == TYPE_CALLABLE; }
	const game_logic::formula_callable* as_callable() const {
		must_be(TYPE_CALLABLE); return callable_; }

	variant operator+(const variant&) const;
	variant operator-(const variant&) const;
	variant operator*(const variant&) const;
	variant operator/(const variant&) const;
	variant operator-() const;

	bool operator==(const variant&) const;
	bool operator!=(const variant&) const;
	bool operator<(const variant&) const;
	bool operator>(const variant&) const;
	bool operator<=(const variant&) const;
	bool operator>=(const variant&) const;

private:

	enum TYPE { TYPE_INT, TYPE_CALLABLE, TYPE_LIST };
	void must_be(TYPE t) const;
	TYPE type_;
	union {
		int int_value_;
		const game_logic::formula_callable* callable_;
		variant_list* list_;
	};
};

struct type_error {
	explicit type_error(const std::string& str) {}
};

#endif
