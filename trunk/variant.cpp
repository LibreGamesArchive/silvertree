#include <cmath>
#include <stdlib.h>
#include <vector>

#include <iostream>

#include "boost/lexical_cast.hpp"

#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
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

void variant::release_resources()
{
	switch(type_) {
	case TYPE_LIST:
		release_list(list_);
		*this = variant();
		break;
	case TYPE_STRING:
		release_string(string_);
		*this = variant();
		break;
	}
}

variant variant::deep_copy() const
{
	variant res;
	switch(type_) {
	case TYPE_LIST: {
		variant_list* v = allocate_list(res);
		v->elements.resize(list_->elements.size());
		for(int n = 0; n != v->elements.size(); ++n) {
			v->elements[n] = list_->elements[n].deep_copy();
		}
		return res;
	}
	case TYPE_STRING:
		*allocate_string(res) = *string_;
		return res;
	default:
		res = *this;
		return res;
	}
}

void variant::serialize_to_string(std::string& str) const
{
	switch(type_) {
	case TYPE_INT:
		str += boost::lexical_cast<std::string>(int_value_);
		break;
	case TYPE_CALLABLE:
		std::cerr << "ERROR: attempt to serialize callable variant\n";
		break;
	case TYPE_LIST: {
		str += "[";
		bool first_time = true;
		foreach(const variant& var, list_->elements) {
			if(!first_time) {
				str += ",";
			}
			first_time = false;
			var.serialize_to_string(str);
		}
		str += "]";
		break;
	}
	case TYPE_STRING:
		str += "'";
		str += string_->str;
		str += "'";
		break;
	default:
		assert(false);
	}
}

void variant::serialize_from_string(const std::string& str)
{
	*this = game_logic::formula(str).execute();
}
