#include <iostream>
#include <sstream>
#include <fstream>

#include <cstdlib>
#include <cmath>
#include <ctime>

#include "namegen.hpp"

#ifdef VERBOSE
const bool verbose = 1;
#else
const bool verbose = 0;
#endif

namespace namegen {


float ranfu() {
	return rand()/(static_cast<float>(RAND_MAX));
}

int raniu(int n) {
	return static_cast<int>(n*(rand()/static_cast<float>(RAND_MAX)));
}

float phoneme::transition_prob(const phoneme_ptr p) const {
	const_iterator i = trans_.find(p);
	if(i != trans_.end()) {
		return i->second;
	}
	return 0;
}



const phoneme_ptr phoneme::transition() const {
	const float index = ranfu();
	float accum = 0;

	for(const_iterator i = trans_.begin();
	    i != trans_.end(); ++i) {
		if(index > accum) {
			const float upper = accum + i->second;
			if(index < upper) {
				return i->first;
			}
		}
		accum += i->second;
	}
	return phoneme_ptr(trans_.rbegin()->first);
}

void phoneme::normalise() {
	float sum = 0;
	for(const_iterator i = trans_.begin(); i != trans_.end(); ++i) {
		sum += i->second;
	}
	for(iterator i = trans_.begin(); i != trans_.end(); ++i) {
		i->second /= sum;
	}
}

void typography::gen_transitions(const std::vector<phoneme_ptr>& starts,
				 const std::vector<phoneme_ptr>& ends) {

	for(std::vector<phoneme_ptr>::const_iterator j = starts.begin();
	    j != starts.end(); ++j) {
		for(std::vector<phoneme_ptr>::const_iterator k = ends.begin();
		    k!=ends.end(); ++k) {
			(*j)->set_transition((*k), ranfu());
		}
	}
}

void typography::normalise_phones(const std::vector<phoneme_ptr>& p) {
	for(const_iterator i = p.begin(); i != p.end(); ++i) {
		(*i)->normalise();
	}
}

void typography::set_non_initial(phoneme_ptr p) {
	non_initials_.insert(non_initials_.begin(), p);
}

void typography::set_non_medial(phoneme_ptr p) {
	non_medials_.insert(non_medials_.begin(), p);
}

void typography::set_non_final(phoneme_ptr p) {
	non_finals_.insert(non_finals_.begin(), p);
}

void typography::add_qualifiers(phoneme_ptr p, typo_ptr t,
				const std::vector<std::string>& non_initials,
				const std::vector<std::string>& non_medials,
				const std::vector<std::string>& non_finals) {
	if(std::find(non_initials.begin(), non_initials.end(), p->rep()) != non_initials.end()) {
		t->set_non_initial(p);
	}
	if(std::find(non_medials.begin(), non_medials.end(), p->rep()) != non_medials.end()) {
		t->set_non_medial(p);
	}
	if(std::find(non_finals.begin(), non_finals.end(), p->rep()) != non_finals.end()) {
		t->set_non_final(p);
	}
}

typo_ptr typography::generate_random(const std::vector<std::string>& vowels,
				     const std::vector<std::string>& consonants,
				     const std::vector<std::string>& non_initials,
				     const std::vector<std::string>& non_medials,
				     const std::vector<std::string>& non_finals)
{
	typo_ptr ret(new typography());
	std::vector<phoneme_ptr> v_phones;
	std::vector<phoneme_ptr> c_phones;
	std::vector<std::string>::const_iterator i;

	for(i = vowels.begin();
	    i != vowels.end(); ++i) {
		phoneme_ptr p(new phoneme(*i));
		ret->push_phone(p);
		v_phones.push_back(p);
		add_qualifiers(p, ret, non_initials, non_medials, non_finals);
	}

	for(i = consonants.begin();
	    i != consonants.end(); ++i) {
		phoneme_ptr p(new phoneme(*i));
		ret->push_phone(p);
		c_phones.push_back(p);
		add_qualifiers(p, ret, non_initials, non_medials, non_finals);
	}

	gen_transitions(v_phones, c_phones);
	gen_transitions(c_phones, v_phones);

	normalise_phones(v_phones);
	normalise_phones(c_phones);

	return ret;
}

std::string typography::generate_word(int length) const {
	if(length <= 0) {
		return "";
	}
	if(length == 1) {
		phoneme_ptr p;
		do {
			p = phones_.at(raniu(static_cast<int>(phones_.size())));
		} while(non_initials_.find(p) != non_initials_.end() ||
			non_finals_.find(p) != non_finals_.end());
		return p->rep();
	}

	/* generate initial */
	phoneme_ptr p;
	do {
		int index = raniu(static_cast<int>(phones_.size()));
		p = phones_.at(index);
	} while(non_initials_.find(p) != non_initials_.end());

	std::string ret = p->rep();

	/* generate medials */
	for(int i=1;i<length-1;++i) {
		phoneme_ptr q;
		do {
			q = p->transition();
		} while(non_medials_.find(q) != non_medials_.end());

		ret += q->rep();
		p = q;
	}

	/* generate final */
	{
		phoneme_ptr q;
		do {
			q = p->transition();
		} while(non_finals_.find(q) != non_finals_.end());
		ret += q->rep();
	}

	return ret;
}

}

std::string read_file(const std::string& filename) {
	std::ifstream file(filename.c_str(), std::ios_base::binary);

	if(!file.is_open())
	{
		std::cerr << "Error opening file: "<<filename<<"\n";
		exit(2);
	}

	std::stringstream ss;
	ss << file.rdbuf();

	file.close();
	return ss.str();
}

std::vector< std::string > split(std::string const &val, const std::string& delim)
{
	/* this might be slow but its very convenient so long as you
	   aren't calling it too often */

	std::vector< std::string > res;
	std::string::const_iterator i1 = val.begin();
	std::string::const_iterator i2 = val.begin();

	while (i2 != val.end()) {
		if(delim.find(*i2) != std::string::npos) {
			std::string new_val(i1, i2);
			res.push_back(new_val);
			while((++i2) != val.end() && delim.find(*i2) != std::string::npos) {}
			i1 = i2;
		} else {
			++i2;
		}
	}
	std::string new_val(i1,i2);
	if(!new_val.empty()) {
		res.push_back(new_val);
	}
	return res;
}


void read_clusters(const std::string& filename,
		   std::vector<std::string>& vowels,
		   std::vector<std::string>& consonants,
		   std::vector<std::string>& non_initials,
		   std::vector<std::string>& non_medials,
		   std::vector<std::string>& non_finals) {

	std::string data = read_file(filename);

	std::vector<std::string> lines = split(data, "\n");
	int line_count = 0;
	for(std::vector<std::string>::const_iterator line_iter =  lines.begin();
	    line_iter != lines.end(); ++line_iter) {
		++line_count;
		if(verbose) {
			std::cout << "Parsing line "<<line_count<<": "<< (*line_iter) << "\n";
		}
		std::vector<std::string> words = split((*line_iter), " \t\v\r");
		if(words.size() == 0) {
			std::cerr << "Blank line at "<<filename<<":"<<line_count<<"\n";
			continue;
		}
		if(words.size() == 1) {
			std::cerr << "No descriptor for cluster at "<<filename<<":"<<line_count<<"\n";
			continue;
		}
		if(words.size() > 2) {
			std::cerr << "Garbage after descriptor at "<<filename<<":"<<line_count<<"\n";
		}
		bool is_vowel = false;
		bool is_consonant = false;
		bool is_medial = true;
		bool is_final = true;
		bool is_initial = true;
		for(std::string::const_iterator i = words.back().begin();
		    i != words.back().end(); ++i) {
			switch(*i) {
			case 'v':
				is_vowel = true;
				break;
			case 'c':
				is_consonant = true;
				break;
			case 'i':
				is_initial = false;
				break;
			case 'f':
				is_final = false;
				break;
			case 'm':
				is_medial = false;
				break;
			default:
				std::cerr << "Unknown marker in descriptor '"<<(*i)<<"' at "
					  << filename << ":" <<line_count << "\n";
				break;
			}
		}
		if(!is_vowel && !is_consonant) {
			std::cerr << "Warning: cluster is neither vowel nor consonant at "
				  << filename << ":" << line_count << "\n";
			break;
		}
		if(!is_initial && !is_medial && !is_final) {
			std::cerr << "Warning: cluster can never be used at "
				  << filename << ":" << line_count << "\n";
		}

		if(verbose) {
			std::cout << "Cluster: "<<words.front()<< " ";
		}

		if(is_vowel) {
			if(verbose) {
				std::cout << "(vowel) ";
			}
			vowels.push_back(words.front());
		}
		if(is_consonant) {
			if(verbose) {
				std::cout << "(consonant) ";
			}
			consonants.push_back(words.front());
		}
		if(!is_initial) {
			if(verbose) {
				std::cout << "(non-initial) ";
			}
			non_initials.push_back(words.front());
		}
		if(!is_medial) {
			if(verbose) {
				std::cout << "(non-medial) ";
			}
			non_medials.push_back(words.front());
		}
		if(!is_final) {
			if(verbose) {
				std::cout << "(non-final) ";
			}
			non_finals.push_back(words.front());
		}
		if(verbose) {
			std::cout << "\n";
		}
	}
}

void write_typography(const std::string& filename, namegen::typo_ptr t) {
	std::ofstream out(filename.c_str(), std::ios_base::binary);


	for(namegen::typography::const_iterator i = t->begin_phones();
	    i != t->end_phones(); ++i)
	{
		for(namegen::typography::const_iterator j = t->begin_phones();
		    j != t->end_phones(); ++j) {
			out << (*i)->rep() << " " << (*j)->rep() << " "
			    << (*i)->transition_prob((*j)) << "\n";
		}
	}
	out.close();
}


namegen::typo_ptr read_typography(const std::string& cluster_file, const std::string& filename) {
	std::string data = read_file(filename);

	std::map<std::string, std::map<std::string, float> > phones;

	std::vector<std::string> lines = split(data, "\n\r");
	int line_count = 0;
	for(std::vector<std::string>::const_iterator i = lines.begin();
	    i != lines.end(); ++i) {
		++line_count;
		std::vector<std::string> words = split((*i), " \t\v");
		if(words.size() != 3) {
			std::cerr << "Line is corrupt at "<<filename<<":"<<line_count<<"\n";
			std::cerr << "["<<(*i)<<"] has only "<<words.size()<<" words.\n";
			continue;
		}
		phones[words[0]][words[1]] = static_cast<float>(atof(words[2].c_str()));
	}

	std::vector<std::string> vowels;
	std::vector<std::string> consonants;
	std::vector<std::string> non_initials;
	std::vector<std::string> non_medials;
	std::vector<std::string> non_finals;
	read_clusters(cluster_file, vowels, consonants, non_initials, non_medials, non_finals);

	std::map<std::string, namegen::phoneme_ptr> phone_map;

	namegen::typo_ptr ret(new namegen::typography());
	for(std::map<std::string, std::map<std::string,float> >::const_iterator i = phones.begin();
	    i != phones.end(); ++i) {
		namegen::phoneme_ptr p(new namegen::phoneme(i->first));
		ret->push_phone(p);
		phone_map[i->first] = p;
		namegen::typography::add_qualifiers(p, ret, non_initials, non_medials, non_finals);
	}
	for(std::map<std::string, std::map<std::string,float> >::const_iterator i = phones.begin();
	    i != phones.end(); ++i) {
		for(std::map<std::string,float>::const_iterator j = i->second.begin();
		    j != i->second.end(); ++j) {
			phone_map[i->first]->set_transition(phone_map[j->first], j->second);
			if(verbose) {
				std::cout << "Set transition "<<(i->first)<<" -> "<<j->first<<" = "<<j->second<<"\n";
			}
		}
	}
	return ret;
}

namegen::typo_ptr make_language(const std::string& cluster_file,
				int num_vowels, int num_consonants) {
	std::vector<std::string> vowels;
	std::vector<std::string> consonants;
	std::vector<std::string> non_initials;
	std::vector<std::string> non_medials;
	std::vector<std::string> non_finals;

	if(verbose) {
		std::cout << "Constructing language\n";
	}

	read_clusters(cluster_file, vowels, consonants,
		      non_initials, non_medials, non_finals);

	int vsize = static_cast<int>(vowels.size()) - num_vowels;
	if(vsize < 0) vsize = 0;
	for(int i =0 ;i<vsize; ++i) {
		std::vector<std::string>::iterator iter = vowels.begin();
		vowels.erase(iter + namegen::raniu(static_cast<int>(vowels.size())));
	}
	int csize = static_cast<int>(consonants.size()) - num_consonants;
	if(csize < 0) csize = 0;
	for(int i =0 ;i<csize; ++i) {
		std::vector<std::string>::iterator iter = consonants.begin();
		consonants.erase(iter + namegen::raniu(static_cast<int>(consonants.size())));
	}

	if(verbose) {
		std::cout << consonants.size() << " consonant segments and "<< vowels.size() << " vowel segments.\n";
	}

	namegen::typo_ptr t = namegen::typography::generate_random(vowels, consonants,
								   non_initials, non_medials,
								   non_finals);
	return t;
}

bool file_exists(const std::string& name) {
	std::ifstream file(name.c_str(), std::ios_base::binary);
	if(file.rdstate() != 0) {
		return false;
	}

	file.close();
	return true;
}

int main(int argc, char **argv) {
	srand(static_cast<unsigned int>(time(NULL)));

	if(argc < 3) {
		std::cerr << "Usage: "<<argv[0]<<" <cluster file> <output file> [<minlen> <maxlen> <vowels> <consonants>]\n";
		return -1;
	}
	int num_vowels = 5;
	int num_consonants = 10;
	int min_len = 2;
	int max_len = 6;
	if(argc > 3) {
		min_len = atoi(argv[3]);
	}
	if(argc > 4) {
		max_len = atoi(argv[4]);
	}
	if(argc > 5) {
		 num_vowels = atoi(argv[5]);
	}
	if(argc > 6) {
		num_consonants = atoi(argv[6]);
	}

	namegen::typo_ptr t;

	if(file_exists(argv[2])) {
		t = read_typography(argv[1], argv[2]);
	} else {
		t = make_language(argv[1], num_vowels, num_consonants);
	}

	std::set<std::string> words;
	const int total = 150;
	const int columns = 3;

	std::cout << "Generating "<< total << " names\n";
	for(int i=0;i<total;++i) {
		std::string w;
		int retries = 1000000;
		do {
			w = t->generate_word(namegen::raniu(max_len - min_len)+min_len);
		} while(words.find(w) != words.end() &&
			retries-- > 0);
		if(retries <= 0) {
			std::cerr << "No more legal names!\n";
			break;
		}
		words.insert(words.begin(), w);
	}

	int count = 0;
	for(std::set<std::string>::iterator i = words.begin();
	    i != words.end(); ++i) {

		if(count < 9) {
			std::cout << "  ";
		} else if(count < 99) {
			std::cout <<  " ";
		}
		std::cout << (count+1) << ": " << (*i);
		for(int j=0; j < (16 -static_cast<int>(i->size())); ++j) {
			std::cout << ' ';
		}

		if(count % columns != (columns-1)) {
			std::cout << "\t";
		} else {
			std::cout << "\n";
		}
		++count;
	}

	if(words.size() % columns != 0) {
		std::cout << "\n";
	}

	std::cout << "\n";

	if(!file_exists(argv[2])) {
		write_typography(argv[2], t);
	}

	return 0;
}
