#include <cmath>
#include <stdlib.h>
#include <vector>

#include <iostream>

#include "formatter.hpp"
#include "variant.hpp"

struct variant_list {
	std::vector<variant> elements;
};

struct variant_string {
	std::string str;
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

variant_string* variant::allocate_string(variant& v)
{
	variant_string* res = new variant_string;
	v.type_ = TYPE_STRING;
	v.string_ = res;
	return res;
}

void variant::release_string(variant_string* s)
{
	delete s;
}

variant::variant(int n) : type_(TYPE_INT), int_value_(n)
{}

variant::variant(const game_logic::formula_callable* callable)
	: type_(TYPE_CALLABLE), callable_(callable)
{
	assert(callable_);
}

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

const variant& variant::set_string(const std::string& str)
{
	must_be(TYPE_STRING);
	assert(string_);
	string_->str = str;
	return *this;
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
	return variant(static_cast<int>(pow(int_value_, v.int_value_)));
}

variant variant::operator-() const
{
	must_be(TYPE_INT);
	return variant(-int_value_);
}

bool variant::operator==(const variant& v) const
{
	if(type_ == TYPE_STRING) {
		v.must_be(TYPE_STRING);
		std::cerr << "compare: '" << string_->str << "' vs '" << v.string_->str << "'\n";
		return string_->str == v.string_->str;
	}

	must_be(TYPE_INT);
	v.must_be(TYPE_INT);
	return int_value_ == v.int_value_;
}

bool variant::operator!=(const variant& v) const
{
	return !operator==(v);
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
