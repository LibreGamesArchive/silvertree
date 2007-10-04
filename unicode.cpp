#include <cassert>
#include <set>
#include "unicode.hpp"

namespace gui {
namespace text {


namespace {

std::set<Uint32> punctuation, whitespace;
bool punctuation_init = false;
bool whitespace_init = false;

inline void insert_punc(Uint32 x) {
	punctuation.insert(x);
}

inline void insert_range_punc(Uint32 start, Uint32 end) 
{
	for(Uint32 i = start; i <= end; ++i) 
	{
		insert_punc(i);
	}
}

void init_punctuation() 
{
	/* the following is derived from the Unicode Standard Annex #14
	   on the line breaking properties of codepoints (v5.0)*/

	/* hyphens */
	insert_punc(0x00AD); /* soft hyphen */
	insert_punc(0x058A); /* armenian hyphen */
	insert_punc(0x2010); /* hyphen */
	insert_punc(0x2012); /* figure dash */
	insert_punc(0x2013); /* en dash */

	/* visible word dividers */
	insert_punc(0x05BE); /* Hebrew maqaf */
	insert_punc(0x0F0B); /* Tibetan tsheg */
	insert_punc(0x1361); /* ethiopic wordspace */
	insert_punc(0x17D5); /* Khmer bariyoosan */
	insert_range_punc(0x10100, 0x10102); /* aegean symbols */
	insert_punc(0x1039F); /* Ugaritic word divider */
	insert_punc(0x103D0); /* Old persian word divider */
	insert_punc(0x12470); /* Cuneiform punctuation sign */

	insert_punc(0x2027); /* hyphenation point */
	insert_punc(0x007c); /* vertical line */

	/* old word separators */
	insert_range_punc(0x16EB, 0x16ED); /* runic */
	insert_punc(0x2056);
	insert_range_punc(0x2058, 0x205B); 
	insert_punc(0x205D);
	insert_punc(0x205E);

	/* dandas */
	insert_range_punc(0x0964, 0x0965); /* devanagari */
	insert_punc(0x0E5A); /* thai */
	insert_range_punc(0x104A, 0x104B); /* myanmar */
	insert_range_punc(0x1735, 0x1736); /* phillipine */
	insert_range_punc(0x17D4, 0x17D5); /* khmer */
	insert_punc(0x17D8);
	insert_punc(0x17DA);
	insert_range_punc(0x10A56, 0x10A57); /* kharoshthi */

	/* tibetan */
	insert_punc(0x0F85); /* paluta */
	insert_punc(0x0F34); /* bsdus rtags */
	insert_punc(0x0F7F); /* rnam bcad */
	insert_range_punc(0x0FBE, 0x0FBF); /* ku ru kha */

	/* terminating punctuation */
	insert_range_punc(0x1802, 0x1805); /* mongolian */
	insert_range_punc(0x1808, 0x1809); /* manchu */
	insert_punc(0x1A1E); /* pallawa */
	insert_range_punc(0x2CF9, 0x2CFC); /* nubian */
	insert_range_punc(0x2CFE, 0x2CFF); /* coptic */
	insert_range_punc(0x10A50, 0x10A55); /* kharoshthi */

	insert_punc(0x00B4); /* acute accent */
	insert_punc(0x02C8);
	insert_punc(0x02CC); 

	/* tibetan head letters */
	insert_range_punc(0x0F01, 0x0F04);
	insert_range_punc(0x0F06, 0x0F07);
	insert_range_punc(0x0F09, 0x0F0A);
	insert_range_punc(0x0FD0, 0x0FD1);

	insert_punc(0x1806); /* mongolian soft hyphen */

	insert_punc(0x2014); /* em dash */
	
	insert_punc(0xFFFC); /* object replacement character */

	insert_range_punc(0x3001, 0x3002); /* ideographic punctuation */
	insert_range_punc(0xFE11, 0xFE12); 
	insert_punc(0xFE50);
	insert_punc(0xFE52);
	insert_punc(0xFF0C);
	insert_punc(0xFF0E);
	insert_punc(0xFF61);
	insert_punc(0xFF64);

	insert_punc(0x0021); /* exclamation mark */
	insert_punc(0x003F); /* question mark */
	insert_punc(0x05C6); /* nun hafukha */
	insert_punc(0x060C); /* arabic comma */
	insert_punc(0x061B); /* arabic semicolon */
	insert_range_punc(0x061E, 0x061F); 
	insert_punc(0x066A); 
	insert_punc(0x06D4);
	insert_punc(0x07F9); /* niko exclamation sign */
	insert_range_punc(0x0F0D, 0x0F11); /* tibetan */
	insert_punc(0x0F14); 
	insert_range_punc(0x1944, 0x1945); /* limbu */
	insert_range_punc(0x2762, 0x2763); /* ornaments */
	insert_range_punc(0xA876, 0xA877); /* phags-pa */
	insert_range_punc(0xFE15, 0xFE16); 
	insert_range_punc(0xFE56, 0xFE57); /* small marks */
	insert_punc(0xFF01); 
	insert_punc(0xFF1F); 

	/* tibetan */
	insert_punc(0x0F08);
	insert_punc(0x0F0C);
	insert_punc(0x0F12);

	insert_punc(0x002D); /* hyphen-minus */
	
	insert_punc(0x002C); /* comma */
	insert_punc(0x002E); /* full stop */
	insert_range_punc(0x003A, 0x003B); /* colon, semicolon */
	insert_punc(0x037E); /* greek question mark */
	insert_punc(0x0589); /* armenian full stop */
	insert_punc(0x060D); /* arabic date separator */
	insert_punc(0x07F8); /* nko comma */
	insert_punc(0x2044); /* fraction slash */
	insert_punc(0xFE10); /* vertical comma */
	insert_range_punc(0xFE13, 0xFE14); /* vertical colon, semicolon */

	punctuation_init = true;
}

inline void insert_ws(Uint32 x) {
	whitespace.insert(x);
}

inline void insert_range_ws(Uint32 start, Uint32 end) 
{
	for(Uint32 i = start; i <= end; ++i) 
	{
		insert_ws(i);
	}
}

void init_whitespace() {
	insert_range_ws(0x0009, 0x000D);
	insert_ws(0x0020);
	insert_ws(0x0085);
	insert_ws(0x00A0);
	insert_ws(0x1680);
	insert_ws(0x180E);
	insert_range_ws(0x2000, 0x200A);
	insert_range_ws(0x2028, 0x2029);
	insert_ws(0x202F);
	insert_ws(0x205F);
	insert_ws(0x3000);

	whitespace_init = true;
}

} /* anon namespace */

bool is_breakable_p::operator()(Uint32 x) {
	if(!whitespace_init) {
		init_whitespace();
	}
	if(whitespace.find(x) != whitespace.end()) {
		return true;
	}
	if(!punctuation_init) {
		init_punctuation();
	}
	return punctuation.find(x) != punctuation.end();
}

bool is_whitespace_p::operator()(Uint32 x) 
{
	if(!whitespace_init) {
		init_whitespace();
	}
	return whitespace.find(x) != whitespace.end();
}

bool is_punctuation_p::operator()(Uint32 x) 
{
	if(!punctuation_init) {
		init_punctuation();
	}
				   
	return punctuation.find(x) != punctuation.end();
}

std::string utf32_to_utf8(const std::vector<Uint32>& vec) {
	std::string ret;
	for(std::vector<Uint32>::const_iterator i = vec.begin();
	    i != vec.end(); ++i) 
	{
		Uint32 v = *i;

		if(v<= 0x7F) {
			ret += static_cast<char>(v);
			continue;
		}
		if(v<=0x7FF) {
			Uint8 high_val = 0xC2 | (v >> 6) ;
			Uint8 low_val = 0x80 | (v & 0x3F);
			ret += static_cast<char>(high_val);
			ret += static_cast<char>(low_val);
			continue;
		}
		if(v<=0xFFFF) {
			Uint8 high_val = 0xE0 | ( v >> 12);
			Uint8 mid_val = 0x80 | ((v >> 6) & 0x3F);
			Uint8 low_val = 0x80 | (v & 0x3F);
			ret += static_cast<char>(high_val);
			ret += static_cast<char>(mid_val);
			ret += static_cast<char>(low_val);
			continue;
		}
		if(v < 0x10FFFF) {
			Uint8 high_val = 0xF0 | (v >> 18);
			Uint8 midh_val = 0x80 | ((v >> 12) & 0x3F);
			Uint8 midl_val = 0x80 | ((v >> 6) & 0x3F);
			Uint8 low_val = 0x80 | (v & 0x3F);
			ret += static_cast<char>(high_val);
			ret += static_cast<char>(midh_val);
			ret += static_cast<char>(midl_val);
			ret += static_cast<char>(low_val);
			continue;
		} else {
			/* out of range - either data is corrupt or 
			   unicode 4.0 is not being used */
			assert(false);
		}
	}
	return ret;
}

std::vector<Uint32> utf8_to_utf32(const std::string& str) {
	std::vector<Uint32> ret;
	for(std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
		Uint8 c = static_cast<Uint8>(*i);
		if(!(c & 0x80)) { /* no high bit is set => no special treatment */
			ret.push_back(c);
			continue;
		}
		Uint32 data = 0;
		int data_bytes = 0;
		if(c & 0xF0) { /* 4 byte */
			data_bytes = 3;
			data += (c & 0x07) << (data_bytes * 6);
		} else if(c & 0xE0) { /* 3 byte */
			data_bytes = 2;
			data += (c & 0x0F) << (data_bytes * 6);
		} else if(c & 0xC2) { /* 2 byte */
			data_bytes = 1;
			data += (c & 0x1F) << (data_bytes * 6);
		} else {
			/* data byte without header */
			assert(false);
		}
		for(int d =data_bytes-1;d >= 0;--d) {
			Uint8 c = static_cast<Uint8>(*(++i));
			/* check this is a data byte */
			assert((c >> 6) == 2);
			data += (c & 0x3F) << (d * 6);
		}
		ret.push_back(data);
	}
	return ret;
}

}
}
