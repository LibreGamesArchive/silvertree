
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

map_formula_callable::map_formula_callable(
    const formula_callable* fallback) : fallback_(fallback)
{}

map_formula_callable& map_formula_callable::add(const std::string& key,
                                                int value)
{
	values_[key] = value;
	return *this;
}

int map_formula_callable::get_value(const std::string& key) const
{
	return map_get_value_default(values_, key,
	        fallback_ ? fallback_->query_value(key) : 0);
}

class formula_expression {
public:
	int evaluate(const formula_callable& variables) const {
		return execute(variables);
	}
private:
	virtual int execute(const formula_callable& variables) const = 0;
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
	int execute(const formula_callable& variables) const {
		const int i = args()[0]->evaluate(variables) ? 1 : 2;
		return args()[i]->evaluate(variables);
	}
};

class abs_function : public function_expression {
public:
	explicit abs_function(const args_list& args)
	     : function_expression(args, 1, 1)
	{}

private:
	int execute(const formula_callable& variables) const {
		const int n = args()[0]->evaluate(variables);
		return n >= 0 ? n : -n;
	}
};

class min_function : public function_expression {
public:
	explicit min_function(const args_list& args)
	     : function_expression(args, 2, 2)
	{}

private:
	int execute(const formula_callable& variables) const {
		const int n1 = args()[0]->evaluate(variables);
		const int n2 = args()[1]->evaluate(variables);
		return n1 < n2 ? n1 : n2;
	}
};

class max_function : public function_expression {
public:
	explicit max_function(const args_list& args)
	     : function_expression(args, 2, 2)
	{}

private:
	int execute(const formula_callable& variables) const {
		const int n1 = args()[0]->evaluate(variables);
		const int n2 = args()[1]->evaluate(variables);
		return n1 > n2 ? n1 : n2;
	}
};

class rgb_function : public function_expression {
public:
	explicit rgb_function(const args_list& args)
	     : function_expression(args, 3, 3)
	{}

private:
	int execute(const formula_callable& variables) const {
		return 10000*
		 std::min<int>(99,std::max<int>(0,args()[0]->evaluate(variables))) +
		 std::min<int>(99,std::max<int>(0,args()[1]->evaluate(variables)))*100+
		 std::min<int>(99,std::max<int>(0,args()[2]->evaluate(variables)));
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
	int execute(const formula_callable& variables) const {
		const int value = args()[0]->evaluate(variables);
		const int begin = args()[1]->evaluate(variables);
		const int end = args()[3]->evaluate(variables);
		if(value < begin || value > end) {
			return 0;
		}
		const int val1 = args()[2]->evaluate(variables);
		const int val2 = args()[4]->evaluate(variables);
		return transition(begin, val1, end, val2, value);
	}
};

class color_transition_function : public function_expression {
public:
	explicit color_transition_function(const args_list& args)
			: function_expression(args, 5)
	{}
private:
	int execute(const formula_callable& variables) const {
		const int value = args()[0]->evaluate(variables);
		int begin = args()[1]->evaluate(variables);
		int end = -1;
		int n = 3;
		while(n < args().size()) {
			end = args()[n]->evaluate(variables);
			if(value >= begin && value <= end) {
				break;
			}

			begin = end;
			n += 2;
		}

		if(value < begin || value > end) {
			return 0;
		}
		const int val1 = args()[n-1]->evaluate(variables);
		const int val2 = args()[n+1 < args().size() ? n+1 : n]->
		                                         evaluate(variables);
		const int r1 = (val1/10000)%100;
		const int g1 = (val1/100)%100;
		const int b1 = (val1)%100;
		const int r2 = (val2/10000)%100;
		const int g2 = (val2/100)%100;
		const int b2 = (val2)%100;

		const int r = transition(begin,r1,end,r2,value);
		const int g = transition(begin,g1,end,g2,value);
		const int b = transition(begin,b1,end,b2,value);
		return std::min<int>(99,std::max<int>(0,r))*100*100 +
		       std::min<int>(99,std::max<int>(0,g))*100+
		       std::min<int>(99,std::max<int>(0,b));
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
	int execute(const formula_callable& variables) const {
		const int res = operand_->evaluate(variables);
		switch(op_) {
		case NOT: return res ? 0 : 1;
		case SUB: return -res;
		default: assert(false);
		}
	}
	enum OP { NOT, SUB };
	OP op_;
	expression_ptr operand_;
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
	int execute(const formula_callable& variables) const {
		const int left = left_->evaluate(variables);
		const int right = right_->evaluate(variables);
		switch(op_) {
		case AND: return left && right ? 1 : 0;
		case OR:  return left || right ? 1 : 0;
		case ADD: return left + right;
		case SUB: return left - right;
		case MUL: return left * right;
		case DIV: return right == 0 ? 0 : left / right;
		case EQ:  return left == right ? 1 : 0;
		case NEQ: return left != right ? 1 : 0;
		case LTE: return left <= right ? 1 : 0;
		case GTE: return left >= right ? 1 : 0;
		case LT:  return left < right ? 1 : 0;
		case GT:  return left > right ? 1 : 0;
		default: assert(false);
		}
	}

	enum OP { AND, OR, NEQ, LTE, GTE, GT='>', LT='<', EQ='=',
	          ADD='+', SUB='-', MUL='*', DIV='/' };

	OP op_;
	expression_ptr left_, right_;
};

class identifier_expression : public formula_expression {
public:
	explicit identifier_expression(const std::string& id) : id_(id)
	{}
private:
	int execute(const formula_callable& variables) const {
		return variables.query_value(id_);
	}
	std::string id_;
};

class integer_expression : public formula_expression {
public:
	explicit integer_expression(int i) : i_(i)
	{}
private:
	int execute(const formula_callable& variables) const {
		return i_;
	}

	int i_;
};

using namespace formula_tokenizer;
int operator_precedence(const token& t)
{
	static std::map<std::string,int> precedence_map;
	if(precedence_map.empty()) {
		precedence_map["and"]  = 1;
		precedence_map["or"]   = 1;
		precedence_map["="]    = 2;
		precedence_map["!="]   = 2;
		precedence_map["<"]    = 2;
		precedence_map[">"]    = 2;
		precedence_map["<="]   = 2;
		precedence_map[">="]   = 2;
		precedence_map["+"]    = 3;
		precedence_map["-"]    = 3;
		precedence_map["*"]    = 4;
		precedence_map["/"]    = 4;
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

	return expression_ptr(new operator_expression(
							     std::string(op->begin,op->end),
	                             parse_expression(i1,op),
								 parse_expression(op+1,i2)));
}

}

formula::formula(const std::string& str)
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

	expr_ = parse_expression(&tokens[0],&tokens[0] + tokens.size());
}

int formula::execute(const formula_callable& variables) const
{ 
	return expr_->evaluate(variables);
}
		
}

#ifdef UNIT_TEST_FORMULA
using namespace game_logic;
class mock_char : public formula_callable {
	int get_value(const std::string& key) const {
		if(key == "strength") {
			return 15;
		} else if(key == "agility") {
			return 12;
		}

		return 10;
	}
};
int main()
{
	mock_char c;
	try {
		assert(formula("strength").execute(c) == 15);
		assert(formula("17").execute(c) == 17);
		assert(formula("strength/2 + agility").execute(c) == 19);
		assert(formula("(strength+agility)/2").execute(c) == 13);
		assert(formula("strength > 12").execute(c) == 1);
		assert(formula("strength > 18").execute(c) == 0);
		assert(formula("if(strength > 12, 7, 2)").execute(c) == 7);
		assert(formula("if(strength > 18, 7, 2)").execute(c) == 2);
		assert(formula("2 and 1").execute(c) == 1);
		assert(formula("2 and 0").execute(c) == 0);
		assert(formula("2 or 0").execute(c) == 1);
		assert(formula("-5").execute(c) == -5);
		assert(formula("not 5").execute(c) == 0);
		assert(formula("not 0").execute(c) == 1);
		assert(formula("abs(5)").execute(c) == 5);
		assert(formula("abs(-5)").execute(c) == 5);
		assert(formula("min(3,5)").execute(c) == 3);
		assert(formula("min(5,2)").execute(c) == 2);
		assert(formula("max(3,5)").execute(c) == 5);
		assert(formula("max(5,2)").execute(c) == 5);
	} catch(formula_error& e) {
		std::cerr << "parse error\n";
	}
}
#endif
