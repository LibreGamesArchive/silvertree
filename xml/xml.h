#ifndef XML_H_INCLUDED
#define XML_H_INCLUDED

typedef enum {
	XML_TOKEN_BEGIN_ELEMENT,
	XML_TOKEN_END_ELEMENT,
	XML_TOKEN_ATTR_NAME,
	XML_TOKEN_ATTR_VALUE,
	XML_TOKEN_TEXT,
	XML_TOKEN_END_DOCUMENT,
	XML_TOKEN_ERROR,
} XML_TOKEN_TYPE;

typedef enum {
	XML_STATE_FIND_ELEMENT,
	XML_STATE_FIND_ATTRIBUTE,
	XML_STATE_FIND_VALUE,
	XML_STATE_ERROR,
	XML_STATE_END_DOCUMENT,
} XML_PARSER_STATE;

typedef struct {
	XML_TOKEN_TYPE type;
	const char* str;
	int length;
} XML_TOKEN;

#define XML_TOKEN_EQUALS(token,s) \
	(strlen(s) == token.length && memcmp(token.str,s,token.length) == 0)

typedef struct {
	const char* pos;
	XML_PARSER_STATE state;
	int elements;
	XML_TOKEN pushed_back_token;
	int has_token;
} XML_PARSER;

#ifdef __cplusplus
extern "C" {
#endif

void xml_init_parser(XML_PARSER* parser, const char* document);
int xml_get_token(XML_PARSER* parser, XML_TOKEN* token);
int xml_get_attr(XML_PARSER* parser, XML_TOKEN* attr, XML_TOKEN* value);
int xml_get_child(XML_PARSER* parser, XML_TOKEN* child);
void xml_skip_attributes(XML_PARSER* parser);
void xml_skip_element(XML_PARSER* parser);

#ifdef __cplusplus
}
#endif

#endif
