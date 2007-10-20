#ifndef UNICODE_HPP_INCLUDED
#define UNICODE_HPP_INCLUDED

#include <string>
#include <vector>
#include "SDL.h"

namespace gui {
namespace text {

class is_whitespace_p {
public:
	bool operator()(Uint32 x);
};

class is_punctuation_p {
public:
	bool operator()(Uint32 x);
};

class is_breakable_p {
public:
	bool operator()(Uint32 x);
};

std::string utf32_to_utf8(const std::vector<Uint32>& vec);
std::vector<Uint32> utf8_to_utf32(const std::string& str);

}
}

#endif
