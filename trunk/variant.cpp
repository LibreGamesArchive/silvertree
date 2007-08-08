#include "formatter.hpp"
#include "variant.hpp"

variant::variant(int n) : type_(TYPE_INT), int_value_(n)
{}

variant variant::operator+(const variant& v) const
{
	return variant(int_value_ + v.int_value_);
}

variant variant::operator-(const variant& v) const
{
	return variant(int_value_ - v.int_value_);
}

variant variant::operator*(const variant& v) const
{
	return variant(int_value_ * v.int_value_);
}

variant variant::operator/(const variant& v) const
{
	if(v.int_value_ == 0) {
		throw type_error(formatter() << "divide by zero error");
	}

	return variant(int_value_ / v.int_value_);
}

variant variant::operator-() const
{
	return variant(-int_value_);
}

bool variant::operator==(const variant& v) const
{
	return int_value_ == v.int_value_;
}

bool variant::operator!=(const variant& v) const
{
	return int_value_ != v.int_value_;
}

bool variant::operator<=(const variant& v) const
{
	return int_value_ <= v.int_value_;
}

bool variant::operator>=(const variant& v) const
{
	return int_value_ >= v.int_value_;
}

bool variant::operator<(const variant& v) const
{
	return int_value_ < v.int_value_;
}

bool variant::operator>(const variant& v) const
{
	return int_value_ > v.int_value_;
}
