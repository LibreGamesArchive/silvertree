#ifndef NAMEGEN_HPP_INCLUDED
#define NAMEGEN_HPP_INCLUDED

#include <map>
#include <set>
#include <string>
#include <vector>

#include "boost/shared_ptr.hpp"

namespace namegen {

class phoneme;
typedef boost::shared_ptr<phoneme> phoneme_ptr;

class phoneme {
public:
	typedef std::map<phoneme_ptr,float>::const_iterator const_iterator;

	phoneme(std::string rep) : rep_(rep) {}
	float transition_prob(const phoneme_ptr next) const;
	const phoneme_ptr transition() const;
	std::string rep() const { return rep_; }
	void set_transition(const phoneme_ptr next, float val) { trans_[next] = val; }
	void normalise();
private:
	typedef std::map<phoneme_ptr,float>::iterator iterator;
	std::string rep_;
	std::map<phoneme_ptr,float> trans_;
};


class typography;
typedef boost::shared_ptr<typography> typo_ptr;

class typography {
public:
	typedef std::vector<phoneme_ptr>::const_iterator const_iterator;

	static typo_ptr generate_random(const std::vector<std::string>& vowels, 
					const std::vector<std::string>& consonants, 
					const std::vector<std::string>& non_initials, 
					const std::vector<std::string>& non_medials, 
					const std::vector<std::string>& non_finals);
	std::string generate_word(int length) const;
	const_iterator begin_phones() const { return phones_.begin(); }
	const_iterator end_phones() const { return phones_.end(); }

	void set_non_initial(phoneme_ptr p);
	void set_non_medial(phoneme_ptr p);
	void set_non_final(phoneme_ptr p);
	static void add_qualifiers(phoneme_ptr p, 
				   typo_ptr t, 
				   const std::vector<std::string>& non_initials,
				   const std::vector<std::string>& non_medials,
				   const std::vector<std::string>& non_finals);
	static void normalise_phones(const std::vector<phoneme_ptr>& phones);
	void push_phone(phoneme_ptr p) { phones_.push_back(p); }
private:
	static void gen_transitions(const std::vector<phoneme_ptr>& starts, 
				    const std::vector<phoneme_ptr>& ends);

	std::set<phoneme_ptr> non_initials_, non_medials_, non_finals_;
	std::vector<phoneme_ptr> phones_;
};

}

#endif
