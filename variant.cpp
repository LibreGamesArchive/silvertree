#include <cmath>
#include <set>
#include <stdlib.h>
#include <vector>

#include <iostream>

#include "boost/lexical_cast.hpp"

#include "foreach.hpp"
#include "formatter.hpp"
#include "formula.hpp"
#include "formula_callable.hpp"
#include "variant.hpp"

struct variant_list {
	variant_list() : refcount(0)
	{}
	std::vector<variant> elements;
	int refcount;
};

struct variant_string {
	variant_string() : refcount(0)
	{}
	std::string str;
	int refcount;
};

void variant::increment_refcount()
{
	switch(type_) {
	case TYPE_LIST:
		++list_->refcount;
		break;
	case TYPE_STRING:
		++string_->refcount;
		break;
	case TYPE_CALLABLE:
		intrusive_ptr_add_ref(callable_);
		break;
	}
}

void variant::release()
{
	switch(type_) {
	case TYPE_LIST:
		if(--list_->refcount == 0) {
			delete list_;
		}
		break;
	case TYPE_STRING:
		if(--string_->refcount == 0) {
			delete string_;
		}
		break;
	case TYPE_CALLABLE:
		intrusive_ptr_release(callable_);
		break;
	}
}

variant::variant(int n) : type_(TYPE_INT), int_value_(n)
{}

variant::variant(const game_logic::formula_callable* callable)
	: type_(TYPE_CALLABLE), callable_(callable)
{
	assert(callable_);
	increment_refcount();
}

variant::variant(std::vector<variant>* array)
    : type_(TYPE_LIST)
{
	assert(array);
	list_ = new variant_list;
	list_->elements.swap(*array);
}

variant::variant(const std::string& str)
	: type_(TYPE_STRING)
{
	string_ = new variant_string;
	string_->str = str;
}

variant::variant(const variant& v)
{
	memcpy(this, &v, sizeof(v));
	increment_refcount();
}

const variant& variant::operator=(const variant& v)
{
	if(&v != this) {
		release();
		memcpy(this, &v, sizeof(v));
		increment_refcount();
	}
	return *this;
}

const variant& variant::operator[](size_t n) const
{
	if(type_ == TYPE_CALLABLE) {
		assert(n == 0);
		return *this;
	}

	must_be(TYPE_LIST);
	assert(list_);
	if(n >= list_->elements.size()) {
		throw type_error("invalid index");
	}

	return list_->elements[n];
}

size_t variant::num_elements() const
{
	if(type_ == TYPE_CALLABLE) {
		return 1;
	}

	must_be(TYPE_LIST);
	assert(list_);
	return list_->elements.size();
}

const std::string& variant::as_string() const
{
	must_be(TYPE_STRING);
	assert(string_);
	return string_->str;
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

std::string variant::string_cast() const
{
	switch(type_) {
	case TYPE_INT:
		return boost::lexical_cast<std::string>(int_value_);
	case TYPE_CALLABLE:
		return "(object)";
	case TYPE_LIST: {
		std::string res = "";
		foreach(const variant& var, list_->elements) {
			if(!res.empty()) {
				res += ", ";
			}

			res += var.string_cast();
		}

		return res;
	}

	case TYPE_STRING:
		return string_->str;
	default:
		assert(false);
	}
}
