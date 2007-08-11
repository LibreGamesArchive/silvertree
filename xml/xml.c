#include <ctype.h>
#include <string.h>

#include "xml.h"

#include <stdio.h>

void xml_init_parser(XML_PARSER* parser, const char* document)
{
	parser->pos = document;
	parser->state = XML_STATE_FIND_ELEMENT;
	parser->elements = 0;
	parser->has_token = 0;
}

#define SET_ERROR(err) \
	token->type = XML_TOKEN_ERROR; \
	token->str = err; \
	token->length = strlen(err); \
	parser->state = XML_STATE_ERROR;

#define ERROR_AND_RETURN(cond,err) \
	if(cond) { \
		SET_ERROR(err); \
		return 0; \
	}

int xml_get_token(XML_PARSER* parser, XML_TOKEN* token)
{
	if(parser->state == XML_STATE_ERROR) {
		token->str = "";
		token->length = 0;
		token->type = XML_TOKEN_ERROR;
		return 0;
	}

	if(parser->state == XML_STATE_END_DOCUMENT) {
		token->type = XML_TOKEN_END_DOCUMENT;
		return 0;
	}

	if(parser->has_token) {
		parser->has_token = 0;
		*token = parser->pushed_back_token;
		return token->type != XML_TOKEN_END_DOCUMENT &&
		       token->type != XML_TOKEN_ERROR;
	}

	if(parser->state == XML_STATE_FIND_ELEMENT) {
		int text_found = 0;
		const char* begin = parser->pos;
		while(*parser->pos && *parser->pos != '<') {
			if(isalnum(*parser->pos) || ispunct(*parser->pos)) {
				text_found = 1;
			}

			++parser->pos;
		}

		if(!*parser->pos) {
			ERROR_AND_RETURN(parser->elements, "unexpected end of document");
			ERROR_AND_RETURN(text_found, "text found at top level");

			parser->state = XML_STATE_END_DOCUMENT;
			token->type = XML_TOKEN_END_DOCUMENT;
			return 0;
		}

		if(text_found) {
			token->type = XML_TOKEN_TEXT;
			token->str = begin;
			token->length = parser->pos - begin;
			return 1;
		} else {
			const char* begin = parser->pos+1;
			if(*begin == '?') {
				parser->pos = strstr(begin,"?>");
				ERROR_AND_RETURN(parser->pos == NULL, "End of DTD not found");

				parser->pos += 2;
				return xml_get_token(parser, token);
			} else if(*begin == '/') {
				--parser->elements;
				++begin;
				parser->pos = strchr(begin,'>');
				ERROR_AND_RETURN(parser->pos == NULL,
				                 "unexpected end of document");
				token->type = XML_TOKEN_END_ELEMENT;
				token->str = begin;
				token->length = parser->pos - token->str;
				++parser->pos;
				return 1;
			} else {
				++parser->elements;
				token->str = parser->pos;
				while(*parser->pos && *parser->pos != ' ' &&
				      *parser->pos != '>' && *parser->pos != '/') {
					++parser->pos;
				}

				ERROR_AND_RETURN(!*parser->pos, "unexpected end of document");

				token->type = XML_TOKEN_BEGIN_ELEMENT;
				token->str = begin;
				token->length = parser->pos - token->str;
				if(*parser->pos == '>') {
					++parser->pos;
					parser->state = XML_STATE_FIND_ELEMENT;
				} else {
					parser->state = XML_STATE_FIND_ATTRIBUTE;
				}

				return 1;
			}
		}
	} else if(parser->state == XML_STATE_FIND_ATTRIBUTE) {
		while(*parser->pos == ' ') {
			++parser->pos;
		}

		ERROR_AND_RETURN(!*parser->pos, "unexpected end of document");

		if(*parser->pos == '/') {
			--parser->elements;
			token->type = XML_TOKEN_END_ELEMENT;
			token->str = "";
			token->length = 0;
			++parser->pos;
			while(*parser->pos != '>') {
				ERROR_AND_RETURN(!*parser->pos, "unexpected end of document");
				ERROR_AND_RETURN(isalpha(*parser->pos) || ispunct(*parser->pos),
				                 "unexpected characters at end of element");
				++parser->pos;
			}
		} else if(isalpha(*parser->pos)) {
			token->type = XML_TOKEN_ATTR_NAME;
			token->str = parser->pos;
			parser->pos = strchr(token->str, '=');
			ERROR_AND_RETURN(parser->pos == NULL, "unexpected end of document");
			token->length = parser->pos - token->str;
			parser->state = XML_STATE_FIND_VALUE;
			return 1;
		} else if(*parser->pos == '>') {
			++parser->pos;
			parser->state = XML_STATE_FIND_ELEMENT;
			return xml_get_token(parser, token);
		} else {
			SET_ERROR("unexpected characters when searching for attribute");
			return 0;
		}
	} else if(parser->state == XML_STATE_FIND_VALUE) {
		++parser->pos;
		while(*parser->pos && *parser->pos != '"') {
			ERROR_AND_RETURN(!isspace(*parser->pos),
			                  "unexpected character when searching for value");
			++parser->pos;
		}

		ERROR_AND_RETURN(!*parser->pos, "unexpected end of document");
		token->str = parser->pos+1;
		parser->pos = strchr(token->str,'"');
		ERROR_AND_RETURN(parser->pos == NULL, "unexpected end of document");

		token->length = parser->pos - token->str;
		token->type = XML_TOKEN_ATTR_VALUE;
		++parser->pos;
		parser->state = XML_STATE_FIND_ATTRIBUTE;
		return 1;
	} else {
		SET_ERROR("bad state");
		return 0;
	}
}

int xml_get_attr(XML_PARSER* parser, XML_TOKEN* attr, XML_TOKEN* value)
{
	xml_get_token(parser, attr);
	if(attr->type != XML_TOKEN_ATTR_NAME) {
		parser->pushed_back_token = *attr;
		parser->has_token = 1;
		return 0;
	}

	xml_get_token(parser, value);
	if(value->type != XML_TOKEN_ATTR_VALUE) {
		parser->pushed_back_token = *attr;
		parser->has_token = 1;
		return 0;
	}

	return 1;
}

int xml_get_child(XML_PARSER* parser, XML_TOKEN* child)
{
	while(xml_get_token(parser, child)) {
		if(child->type == XML_TOKEN_END_ELEMENT) {
			return 0;
		} else if(child->type == XML_TOKEN_BEGIN_ELEMENT) {
			return 1;
		}
	}

	return 0;
}

void xml_skip_element(XML_PARSER* parser)
{
	XML_TOKEN token;
	int elements = 1;
	while(xml_get_token(parser, &token)) {
		if(token.type == XML_TOKEN_BEGIN_ELEMENT) {
			++elements;
		} else if(token.type == XML_TOKEN_END_ELEMENT) {
			if(--elements == 0) {
				return;
			}
		}
	}
}

void xml_skip_attributes(XML_PARSER* parser)
{
	XML_TOKEN token;
	while(xml_get_token(parser, &token) &&
	      (token.type == XML_TOKEN_ATTR_NAME ||
		   token.type == XML_TOKEN_ATTR_VALUE)) {
	}

	parser->pushed_back_token = token;
	parser->has_token = 1;
}

#ifdef UNIT_TEST_XML
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char** argv)
{
	int n;
	for(n = 1; n < argc; ++n) {
		int fd = open(argv[n], O_RDONLY);
		struct stat stat;
		char* buf;
		int nbytes;
		fstat(fd, &stat);
		buf = (char*)malloc(stat.st_size+1);
		nbytes = read(fd, buf, stat.st_size);
		fprintf(stderr, "read %d/%d bytes\n", nbytes, stat.st_size);
		buf[stat.st_size] = 0;
		close(fd);

		{
			XML_PARSER parser;
			XML_TOKEN token;
			xml_init_parser(&parser, buf);
			while(xml_get_token(&parser, &token)) {
					/*
				const char* types[] = {
					"begin element",
					"end element",
					"attr name",
					"attr value",
					"text",
				};

				fprintf(stderr, "parsed '%s': '", types[token.type]);
				write(2, token.str, token.length);
				fprintf(stderr, "'\n");
				*/
			}

			if(token.type == XML_TOKEN_ERROR) {
				fprintf(stderr, "ERROR: %s\n", token.str);
			}
		}
	}
}
#endif
