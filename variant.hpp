#ifndef VARIANT_HPP_INCLUDED
#define VARIANT_HPP_INCLUDED

#include <boost/shared_ptr.hpp>
#include <string>

namespace game_logic {
class formula_callable;
}

class variant {
public:
	explicit variant(int n=0);

	bool is_int() const { return type_ == TYPE_INT; }
	int as_int() const { return int_value_; }
	bool as_bool() const { return int_value_ != 0; }

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

	enum TYPE { TYPE_INT };
	TYPE type_;
	union {
		int int_value_;
	};
};

struct type_error {
	explicit type_error(const std::string& str) {}
};

#endif
