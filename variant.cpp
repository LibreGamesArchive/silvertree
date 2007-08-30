#include <cmath>
#include <stdlib.h>
#include <vector>

#include "formatter.hpp"
#include "variant.hpp"

struct variant_list {
	std::vector<variant> elements;
};

variant_list* variant::allocate_list(variant& v)
{
	variant_list* res = new variant_list;
	v.type_ = TYPE_LIST;
	v.list_ = res;
	return res;
}

void variant::release_list(variant_list* l)
{
	delete l;
}

variant::variant(int n) : type_(TYPE_INT), int_value_(n)
{}

variant::variant(const game_logic::formula_callable* callable)
	: type_(TYPE_CALLABLE), callable_(callable)
{}

variant& variant::operator[](size_t n)
{
	must_be(TYPE_LIST);
	assert(list_);
	if(n >= list_->elements.size()) {
		throw type_error("invalid index");
	}

	return list_->elements[n];
}

const variant& variant::operator[](size_t n) const
{
	must_be(TYPE_LIST);
	assert(list_);
	if(n >= list_->elements.size()) {
		throw type_error("invalid index");
	}

	return list_->elements[n];
}

void variant::add_element(const variant& v)
{
	must_be(TYPE_LIST);
	assert(list_);
	list_->elements.push_back(v);
}

size_t variant::num_elements() const
{
	must_be(TYPE_LIST);
	assert(list_);
	return list_->elements.size();
}

variant variant::operator+(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return variant(int_value_ + v.int_value_);
}

variant variant::operator-(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return variant(int_value_ - v.int_value_);
}

variant variant::operator*(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return variant(int_value_ * v.int_value_);
}

variant variant::operator/(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	if(v.int_value_ == 0) {
		throw type_error(formatter() << "divide by zero error");
	}

	return variant(int_value_ / v.int_value_);
}

variant variant::operator^(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return variant(pow(int_value_, v.int_value_));
}

variant variant::operator-() const
{
	must_be(TYPE_INT);
	return variant(-int_value_);
}

bool variant::operator==(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return int_value_ == v.int_value_;
}

bool variant::operator!=(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return int_value_ != v.int_value_;
}

bool variant::operator<=(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return int_value_ <= v.int_value_;
}

bool variant::operator>=(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return int_value_ >= v.int_value_;
}

bool variant::operator<(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return int_value_ < v.int_value_;
}

bool variant::operator>(const variant& v) const
{
	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return int_value_ > v.int_value_;
}

void variant::must_be(variant::TYPE t) const
{
	if(type_ != t) {
		throw type_error("type error");
	}
}
