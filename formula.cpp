
/*
   Copyright (C) 2007 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
#include <algorithm>
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <vector>

#include "formula.hpp"
#include "formula_tokenizer.hpp"
#include "map_utils.hpp"

namespace game_logic
{

namespace {
std::vector<variant_list*> memory;
std::vector<variant_string*> memory_strings;

variant new_list()
{
	variant res;
	variant_list* mem = variant::allocate_list(res);
	memory.push_back(mem);
	return res;
}

variant new_string()
{
	variant res;
	variant_string* mem = variant::allocate_string(res);
	memory_strings.push_back(mem);
	return res;
}

}

variant formula_callable::create_list() const
{
	return new_list();
}

variant formula_callable::create_string() const
{
	return new_string();
}

map_formula_callable::map_formula_callable(
    const formula_callable* fallback) : fallback_(fallback)
{}

map_formula_callable& map_formula_callable::add(const std::string& key,
                                                const variant& value)
{
	values_[key] = value;
	return *this;
}

variant map_formula_callable::get_value(const std::string& key) const
{
	return map_get_value_default(values_, key,
	        fallback_ ? fallback_->query_value(key) : variant(0));
}

class formula_expression {
public:
	variant evaluate(const formula_callable& variables) const {
		return execute(variables);
	}
private:
	virtual variant execute(const formula_callable& variables) const = 0;
};

namespace {

class function_expression : public formula_expression {
public:
	typedef std::vector<expression_ptr> args_list;
	explicit function_expression(const args_list& args,
	                    int min_args=-1, int max_args=-1)
	    : args_(args)
	{
		if(min_args != -1 && args_.size() < min_args) {
			std::cerr << "too few arguments\n";
			throw formula_error();
		}

		if(max_args != -1 && args_.size() > max_args) {
			std::cerr << "too many arguments\n";
			throw formula_error();
		}
	}

protected:
	const args_list& args() const { return args_; }
private:
	args_list args_;
};

class if_function : public function_expression {
public:
	explicit if_function(const args_list& args)
	     : function_expression(args, 3, 3)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const int i = args()[0]->evaluate(variables).as_bool() ? 1 : 2;
		return args()[i]->evaluate(variables);
	}
};

class rgb_function : public function_expression {
public:
	explicit rgb_function(const args_list& args)
	     : function_expression(args, 3, 3)
	{}

private:
	variant execute(const formula_callable& variables) const {
		return variant(10000*
		 std::min<int>(99,std::max<int>(0,args()[0]->evaluate(variables).as_int())) +
		 std::min<int>(99,std::max<int>(0,args()[1]->evaluate(variables).as_int()))*100+
		 std::min<int>(99,std::max<int>(0,args()[2]->evaluate(variables).as_int())));
	}
};

namespace {
int transition(int begin, int val1, int end, int val2, int value) {
	if(value < begin || value > end) {
		return 0;
	}

	if(value == begin) {
		return val1;
	} else if(value == end) {
		return val2;
	}

	const int comp1 = val1*(end - value);
	const int comp2 = val2*(value - begin);
	return (comp1 + comp2)/(end - begin);
}
}

class transition_function : public function_expression {
public:
	explicit transition_function(const args_list& args)
			: function_expression(args, 5, 5)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const int value = args()[0]->evaluate(variables).as_int();
		const int begin = args()[1]->evaluate(variables).as_int();
		const int end = args()[3]->evaluate(variables).as_int();
		if(value < begin || value > end) {
			return variant(0);
		}
		const int val1 = args()[2]->evaluate(variables).as_int();
		const int val2 = args()[4]->evaluate(variables).as_int();
		return variant(transition(begin, val1, end, val2, value));
	}
};

class color_transition_function : public function_expression {
public:
	explicit color_transition_function(const args_list& args)
			: function_expression(args, 5)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const int value = args()[0]->evaluate(variables).as_int();
		int begin = args()[1]->evaluate(variables).as_int();
		int end = -1;
		int n = 3;
		while(n < args().size()) {
			end = args()[n]->evaluate(variables).as_int();
			if(value >= begin && value <= end) {
				break;
			}

			begin = end;
			n += 2;
		}

		if(value < begin || value > end) {
			return variant(0);
		}
		const int val1 = args()[n-1]->evaluate(variables).as_int();
		const int val2 = args()[n+1 < args().size() ? n+1 : n]->
		                               evaluate(variables).as_int();
		const int r1 = (val1/10000)%100;
		const int g1 = (val1/100)%100;
		const int b1 = (val1)%100;
		const int r2 = (val2/10000)%100;
		const int g2 = (val2/100)%100;
		const int b2 = (val2)%100;

		const int r = transition(begin,r1,end,r2,value);
		const int g = transition(begin,g1,end,g2,value);
		const int b = transition(begin,b1,end,b2,value);
		return variant(
		       std::min<int>(99,std::max<int>(0,r))*100*100 +
		       std::min<int>(99,std::max<int>(0,g))*100+
		       std::min<int>(99,std::max<int>(0,b)));
	}
};


class abs_function : public function_expression {
public:
	explicit abs_function(const args_list& args)
	     : function_expression(args, 1, 1)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const int n = args()[0]->evaluate(variables).as_int();
		return variant(n >= 0 ? n : -n);
	}
};

class min_function : public function_expression {
public:
	explicit min_function(const args_list& args)
	     : function_expression(args, 2, 2)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const variant n1 = args()[0]->evaluate(variables);
		const variant n2 = args()[1]->evaluate(variables);
		return n1 < n2 ? n1 : n2;
	}
};

class max_function : public function_expression {
public:
	explicit max_function(const args_list& args)
	     : function_expression(args, 2, 2)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const variant n1 = args()[0]->evaluate(variables);
		const variant n2 = args()[1]->evaluate(variables);
		return n1 > n2 ? n1 : n2;
	}
};

class choose_element_function : public function_expression {
public:
	explicit choose_element_function(const args_list& args)
	     : function_expression(args, 2, 2)
	{}

private:
	variant execute(const formula_callable& variables) const {
		const variant items = args()[0]->evaluate(variables);
		int max_index = -1;
		variant max_value;
		for(int n = 0; n != items.num_elements(); ++n) {
			const variant val = args()[1]->evaluate(*items[n].as_callable());
			if(max_index == -1 || val > max_value) {
				max_index = n;
				max_value = val;
			}
		}

		if(max_index == -1) {
			return variant(0);
		} else {
			return items[max_index];
		}
	}
};

class filter_function : public function_expression {
public:
	explicit filter_function(const args_list& args)
	    : function_expression(args, 2, 2)
	{}
private:
	variant execute(const formula_callable& variables) const {
		variant res = new_list();
		const variant items = args()[0]->evaluate(variables);
		for(int n = 0; n != items.num_elements(); ++n) {
			const variant val = args()[1]->evaluate(*items[n].as_callable());
			if(val.as_bool()) {
				res.add_element(items[n]);
			}
		}

		return res;
	}
};

expression_ptr create_function(const std::string& fn,
                               const std::vector<expression_ptr>& args)
{
	if(fn == "if") {
		return expression_ptr(new if_function(args));
	} else if(fn == "abs") {
		return expression_ptr(new abs_function(args));
	} else if(fn == "min") {
		return expression_ptr(new min_function(args));
	} else if(fn == "max") {
		return expression_ptr(new max_function(args));
	} else if(fn == "choose") {
		return expression_ptr(new choose_element_function(args));
	} else if(fn == "filter") {
		return expression_ptr(new filter_function(args));
	} else if(fn == "rgb") {
		return expression_ptr(new rgb_function(args));
	} else if(fn == "transition") {
		return expression_ptr(new transition_function(args));
	} else if(fn == "color_transition") {
		return expression_ptr(new color_transition_function(args));
	} else {
		std::cerr << "no function '" << fn << "'\n";
		throw formula_error();
	}
}

class unary_operator_expression : public formula_expression {
public:
	unary_operator_expression(const std::string& op, expression_ptr arg)
	  : operand_(arg)
	{
		if(op == "not") {
			op_ = NOT;
		} else if(op == "-") {
			op_ = SUB;
		} else {
			std::cerr << "illegal unary operator: '" << op << "'\n";
			throw formula_error();
		}
	}
private:
	variant execute(const formula_callable& variables) const {
		const variant res = operand_->evaluate(variables);
		switch(op_) {
		case NOT: return res.as_bool() ? variant(0) : variant(1);
		case SUB: return -res;
		default: assert(false);
		}
	}
	enum OP { NOT, SUB };
	OP op_;
	expression_ptr operand_;
};

class dot_expression : public formula_expression {
public:
	dot_expression(expression_ptr left, expression_ptr right)
	   : left_(left), right_(right)
	{}
private:
	variant execute(const formula_callable& variables) const {
		const variant left = left_->evaluate(variables);
		return right_->evaluate(*left.as_callable());
	}

	expression_ptr left_, right_;
};

class operator_expression : public formula_expression {
public:
	operator_expression(const std::string& op, expression_ptr left,
	                             expression_ptr right)
	  : op_(OP(op[0])), left_(left), right_(right)
	{
		if(op == ">=") {
			op_ = GTE;
		} else if(op == "<=") {
			op_ = LTE;
		} else if(op == "!=") {
			op_ = NEQ;
		} else if(op == "and") {
			op_ = AND;
		} else if(op == "or") {
			op_ = OR;
		}
	}

private:
	variant execute(const formula_callable& variables) const {
		const variant left = left_->evaluate(variables);
		const variant right = right_->evaluate(variables);
		switch(op_) {
		case AND: return left.as_bool() && right.as_bool() ? variant(1) : variant(0);
		case OR: return left.as_bool() || right.as_bool() ? variant(1) : variant(0);
		case ADD: return left + right;
		case SUB: return left - right;
		case MUL: return left * right;
		case DIV: return left / right;
		case POW: return left ^ right;
		case EQ:  return left == right ? variant(1) : variant(0);
		case NEQ: return left != right ? variant(1) : variant(0);
		case LTE: return left <= right ? variant(1) : variant(0);
		case GTE: return left >= right ? variant(1) : variant(0);
		case LT:  return left < right ? variant(1) : variant(0);
		case GT:  return left > right ? variant(1) : variant(0);
		case DICE:return variant(dice_roll(left.as_int(), right.as_int()));
		default: assert(false);
		}
	}

	static int dice_roll(int num_rolls, int faces) {
		int res = 0;
		while(faces > 0 && num_rolls-- > 0) {
			res += (rand()%faces)+1;
		}
		return res;
	}

	enum OP { AND, OR, NEQ, LTE, GTE, GT='>', LT='<', EQ='=',
	          ADD='+', SUB='-', MUL='*', DIV='/', DICE='d', POW='^' };

	OP op_;
	expression_ptr left_, right_;
};

typedef std::map<std::string,expression_ptr> expr_table;
typedef boost::shared_ptr<expr_table> expr_table_ptr;

class where_variables: public formula_callable {
public:
	where_variables(const formula_callable &base,
			expr_table_ptr table )
		: base_(base), table_(table) { }
private:
	const formula_callable& base_;
	expr_table_ptr table_;

	variant get_value(const std::string& key) const {
		expr_table::iterator i = table_->find(key);
		if(i != table_->end()) {
			return i->second->evaluate(base_);
		} 
		return base_.query_value(key);
	}
};

class where_expression: public formula_expression {
public:
	explicit where_expression(expression_ptr body,
				  expr_table_ptr clauses)
		: body_(body), clauses_(clauses)
	{}
	
private:
	expression_ptr body_;
	expr_table_ptr clauses_;

	variant execute(const formula_callable& variables) const {
		where_variables wrapped_variables(variables, clauses_);
		return body_->evaluate(wrapped_variables);
	}
};


class identifier_expression : public formula_expression {
public:
	explicit identifier_expression(const std::string& id) : id_(id)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variables.query_value(id_);
	}
	std::string id_;
};

class integer_expression : public formula_expression {
public:
	explicit integer_expression(int i) : i_(i)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return variant(i_);
	}

	int i_;
};

class string_expression : public formula_expression {
public:
	explicit string_expression(const std::string& str) : str_(str)
	{}
private:
	variant execute(const formula_callable& variables) const {
		return new_string().set_string(str_);
	}
	std::string str_;
};

using namespace formula_tokenizer;
int operator_precedence(const token& t)
{
	static std::map<std::string,int> precedence_map;
	if(precedence_map.empty()) {
		int n = 0;
		precedence_map["where"] = ++n;
		precedence_map["or"]    = ++n;
		precedence_map["and"]   = ++n;
		precedence_map["="]     = ++n;
		precedence_map["!="]    = n;
		precedence_map["<"]     = n;
		precedence_map[">"]     = n;
		precedence_map["<="]    = n;
		precedence_map[">="]    = n;
		precedence_map["+"]     = ++n;
		precedence_map["-"]     = n;
		precedence_map["*"]     = ++n;
		precedence_map["/"]     = ++n;
		precedence_map["^"]     = ++n;
		precedence_map["d"]     = ++n;
		precedence_map["."]     = ++n;
	}

	assert(precedence_map.count(std::string(t.begin,t.end)));
	return precedence_map[std::string(t.begin,t.end)];
}

expression_ptr parse_expression(const token* i1, const token* i2);

void parse_args(const token* i1, const token* i2,
                std::vector<expression_ptr>* res)
{
	int parens = 0;
	const token* beg = i1;
	while(i1 != i2) {
		if(i1->type == TOKEN_LPARENS) {
			++parens;
		} else if(i1->type == TOKEN_RPARENS) {
			--parens;
		} else if(i1->type == TOKEN_COMMA && !parens) {
			res->push_back(parse_expression(beg,i1));
			beg = i1+1;
		}

		++i1;
	}

	if(beg != i1) {
		res->push_back(parse_expression(beg,i1));
	}
}

void parse_where_clauses(const token* i1, const token * i2,
	 		             expr_table_ptr res) {
	int parens = 0;
	const token *original_i1_cached = i1;
	const token *beg = i1;
	std::string var_name;
	while(i1 != i2) {
		if(i1->type == TOKEN_LPARENS) {
			++parens;
		} else if(i1->type == TOKEN_RPARENS) {
			--parens;
		} else if(!parens) {
			if(i1->type == TOKEN_COMMA) {
				if(var_name.empty()) {
					std::cerr << "There is 'where <expression>,; "
						  << "'where name=<expression>,' was needed.\n";
					throw formula_error();
				}
				(*res)[var_name] = parse_expression(beg,i1);
				beg = i1+1;
				var_name.clear();
			} else if(i1->type == TOKEN_OPERATOR) {
				std::string op_name(i1->begin, i1->end);
				if(op_name == "=") {
					if(beg->type != TOKEN_IDENTIFIER) {
						if(i1 == original_i1_cached) {
							std::cerr<< "There is 'where =<expression'; "
								 << "'where name=<expression>' was needed.\n";
						} else {
							std::cerr<< "There is 'where <expression>=<expression>'; " 
								 << "'where name=<expression>' was needed.\n";
						}
						throw formula_error();
					} else if(beg+1 != i1) {
						std::cerr<<"There is 'where name <expression>=<expression>'; "
							 << "'where name=<expression>' was needed.\n";
						throw formula_error();
					} else if(!var_name.empty()) {
						std::cerr<<"There is 'where name=name=<expression>'; "
							 <<"'where name=<expression>' was needed.\n";
						throw formula_error();
					}
					var_name.insert(var_name.end(), beg->begin, beg->end);
					beg = i1+1;
				}
			}
		}
		++i1;
	}
	if(beg != i1) {
		if(var_name.empty()) {
			std::cerr << "There is 'where <expression>'; "
				  << "'where name=<expression> was needed.\n";
			throw formula_error();
		}
		(*res)[var_name] = parse_expression(beg,i1);
	}
}

expression_ptr parse_expression(const token* i1, const token* i2)
{
	if(i1 == i2) {
		std::cerr << "empty expression\n";
		throw formula_error();
	}

	int parens = 0;
	const token* op = NULL;
	for(const token* i = i1; i != i2; ++i) {
		if(i->type == TOKEN_LPARENS) {
			++parens;
		} else if(i->type == TOKEN_RPARENS) {
			--parens;
		} else if(parens == 0 && i->type == TOKEN_OPERATOR) {
			if(op == NULL || operator_precedence(*op) >
			                 operator_precedence(*i)) {
				op = i;
			}
		}
	}

	if(op == NULL) {
		if(i1->type == TOKEN_LPARENS && (i2-1)->type == TOKEN_RPARENS) {
			return parse_expression(i1+1,i2-1);
		} else if(i2 - i1 == 1) {
			if(i1->type == TOKEN_IDENTIFIER) {
				return expression_ptr(new identifier_expression(
				                 std::string(i1->begin,i1->end)));
			} else if(i1->type == TOKEN_INTEGER) {
				int n = boost::lexical_cast<int>(std::string(i1->begin,i1->end));
				return expression_ptr(new integer_expression(n));
			} else if(i1->type == TOKEN_STRING_LITERAL) {
				return expression_ptr(new string_expression(std::string(i1->begin+1,i1->end-1)));
			}
		} else if(i1->type == TOKEN_IDENTIFIER &&
		          (i1+1)->type == TOKEN_LPARENS &&
				  (i2-1)->type == TOKEN_RPARENS) {
			int nleft = 0, nright = 0;
			for(const token* i = i1; i != i2; ++i) {
				if(i->type == TOKEN_LPARENS) {
					++nleft;
				} else if(i->type == TOKEN_RPARENS) {
					++nright;
				}
			}

			if(nleft == nright) {
				std::vector<expression_ptr> args;
				parse_args(i1+2,i2-1,&args);
				return expression_ptr(
				  create_function(std::string(i1->begin,i1->end),args));
			}
		}

		std::cerr << "could not parse expression\n";
		throw formula_error();
	}

	if(op == i1) {
		return expression_ptr(new unary_operator_expression(
		                         std::string(op->begin,op->end),
								 parse_expression(op+1,i2)));
	}

	const std::string op_name(op->begin,op->end);

	if(op_name == ".") {
		return expression_ptr(new dot_expression(
							     parse_expression(i1,op),
								 parse_expression(op+1,i2)));
	}

	if(op_name == "where") {
		expr_table_ptr table(new expr_table());
		parse_where_clauses(op+1, i2, table);
		return expression_ptr(new where_expression(parse_expression(i1, op),
							   table));
	}

	return expression_ptr(new operator_expression(
							     op_name, parse_expression(i1,op),
								 parse_expression(op+1,i2)));
}

}

formula::formula(const std::string& str) : str_(str)
{
	using namespace formula_tokenizer;

	std::vector<token> tokens;
	std::string::const_iterator i1 = str.begin(), i2 = str.end();
	while(i1 != i2) {
		try {
			tokens.push_back(get_token(i1,i2));
			if(tokens.back().type == TOKEN_WHITESPACE) {
				tokens.pop_back();
			}
		} catch(token_error& e) {
			throw formula_error();
		}
	}

	try {
		expr_ = parse_expression(&tokens[0],&tokens[0] + tokens.size());
	} catch(...) {
		std::cerr << "error parsing formula '" << str << "'\n";
		throw;
	}
}

variant formula::execute(const formula_callable& variables) const
{ 
	size_t n = memory.size();
	size_t str_size = memory_strings.size();
	variant res;
	try {
		res = expr_->evaluate(variables);
	} catch(type_error& e) {
		std::cerr << "formula type error!\n";
	}
	for(size_t m = n; m != memory.size(); ++m) {
		variant::release_list(memory[m]);
	}
	memory.resize(n);

	for(size_t m = str_size; m != memory_strings.size(); ++m) {
		variant::release_string(memory_strings[m]);
	}
	memory_strings.resize(str_size);

	return res;
}

variant formula::execute() const
{
	static map_formula_callable null_callable;
	return execute(null_callable);
}
		
}

#ifdef UNIT_TEST_FORMULA
using namespace game_logic;
class mock_char : public formula_callable {
	variant get_value(const std::string& key) const {
		if(key == "strength") {
			return variant(15);
		} else if(key == "agility") {
			return variant(12);
		}

		return variant(10);
	}
};
class mock_party : public formula_callable {
	variant get_value(const std::string& key) const {
		if(key == "members") {
			variant m = create_list();

			i_[0].add("strength",variant(12));
			i_[1].add("strength",variant(16));
			i_[2].add("strength",variant(14));
			for(int n = 0; n != 3; ++n) {
				m.add_element(variant(&i_[n]));
			}

			return m;
		} else if(key == "char") {
			return variant(&c_);
		} else {
			return variant(0);
		}
	}

	mock_char c_;
	mutable map_formula_callable i_[3];

};

#include <time.h>

int main()
{
	srand(time(NULL));
	mock_char c;
	mock_party p;
	try {
		assert(formula("strength").execute(c).as_int() == 15);
		assert(formula("17").execute(c).as_int() == 17);
		assert(formula("strength/2 + agility").execute(c).as_int() == 19);
		assert(formula("(strength+agility)/2").execute(c).as_int() == 13);
		assert(formula("strength > 12").execute(c).as_int() == 1);
		assert(formula("strength > 18").execute(c).as_int() == 0);
		assert(formula("if(strength > 12, 7, 2)").execute(c).as_int() == 7);
		assert(formula("if(strength > 18, 7, 2)").execute(c).as_int() == 2);
		assert(formula("2 and 1").execute(c).as_int() == 1);
		assert(formula("2 and 0").execute(c).as_int() == 0);
		assert(formula("2 or 0").execute(c).as_int() == 1);
		assert(formula("-5").execute(c).as_int() == -5);
		assert(formula("not 5").execute(c).as_int() == 0);
		assert(formula("not 0").execute(c).as_int() == 1);
		assert(formula("abs(5)").execute(c).as_int() == 5);
		assert(formula("abs(-5)").execute(c).as_int() == 5);
		assert(formula("min(3,5)").execute(c).as_int() == 3);
		assert(formula("min(5,2)").execute(c).as_int() == 2);
		assert(formula("max(3,5)").execute(c).as_int() == 5);
		assert(formula("max(5,2)").execute(c).as_int() == 5);
		assert(formula("char.strength").execute(p).as_int() == 15);
		assert(formula("choose(members,strength).strength").execute(p).as_int() == 16);
		assert(formula("4^2").execute().as_int() == 16);
		assert(formula("2+3^3").execute().as_int() == 29);
		assert(formula("2*3^3+2").execute().as_int() == 56);
		assert(formula("9^3").execute().as_int() == 729);
		assert(formula("x*5 where x=1").execute().as_int() == 5);
		assert(formula("x*(a*b where a=2,b=1) where x=5").execute().as_int() == 10);
		assert(formula("char.strength * ability where ability=3").execute(p).as_int() == 45);
		assert(formula("'abcd' = 'abcd'").execute(p).as_bool() == true);
		assert(formula("'abcd' = 'acd'").execute(p).as_bool() == false);
		const int dice_roll = formula("3d6").execute().as_int();
		assert(dice_roll >= 3 && dice_roll <= 18);
	} catch(formula_error& e) {
		std::cerr << "parse error\n";
	}
}
#endif
