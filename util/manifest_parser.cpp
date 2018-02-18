#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../api/utf8.h"

/*
 * Manifest syntax:
 *
 * [section_type section_name]
 * numeric = 5; # Comment
 * string = "hello, world";
 * identifier = Something;
 */

struct Token {
	char *text;
	size_t bytes;

	enum {
		LEFT_BRACKET, RIGHT_BRACKET, EQUALS, SEMICOLON,
		IDENTIFIER, NUMERIC, STRING, 
		END,
	} type;
};

#define EVENT_END_SECTION (0)
#define EVENT_START_SECTION (1)
#define EVENT_ATTRIBUTE (2)

typedef void (*ParseCallback)(Token attribute, Token section, Token variable, Token value, int event);

static bool NextToken(char *&buffer, Token *token, bool expect = false) {
	Token original = *token;

	try_again:;
	int c = utf8_value(buffer);

	token->text = buffer;
	token->bytes = utf8_length_char(buffer);

	if (c == '[') {
		token->type = Token::LEFT_BRACKET;
	} else if (c == ']') {
		token->type = Token::RIGHT_BRACKET;
	} else if (c == '=') {
		token->type = Token::EQUALS;
	} else if (c == ';') {
		token->type = Token::SEMICOLON;
	} else if (c == 0) {
		token->type = Token::END;
		token->bytes = 0;
	} else if (c == ' ' || c == '\n' || c == '\t') {
		buffer++;
		goto try_again;
	} else if (c == '#') {
		while (*buffer != '\n') buffer++;
		goto try_again;
	} else if (isalpha(c) || c >= 0x80 || c == '_') {
		token->type = Token::IDENTIFIER;
		token->bytes = 0;

		while (isalpha(c) || c >= 0x80 || c == '_' || isdigit(c)) {
			token->bytes += utf8_length_char(buffer);
			buffer = token->text + token->bytes;
			c = utf8_value(buffer);
		}
	} else if (isdigit(c)) {
		token->type = Token::NUMERIC;
		token->bytes = 0;

		while (isdigit(c)) {
			token->bytes += utf8_length_char(buffer);
			buffer = token->text + token->bytes;
			c = utf8_value(buffer);
		}
	} else if (c == '"') {
		token->type = Token::STRING;
		buffer = token->text + token->bytes;
		c = utf8_value(buffer);

		while (c != '"' && c) {
			token->bytes += utf8_length_char(buffer);
			buffer = token->text + token->bytes;
			c = utf8_value(buffer);
		}

		if (!c) {
			printf("Unexpected end of file in string\n");
			return false;
		}

		token->bytes++;
	} else {
		printf("Unexpected character in input: '%c'\n", c);
		return false;
	}

	buffer = token->text + token->bytes;

	if (expect && original.type != token->type) {
		printf("Unexpected token type. Found '%.*s'\n", token->bytes, token->text);
		return false;
	}

	return true;
}

static bool ParseManifest(char *text, ParseCallback callback) {
	Token token, section, attribute;
	attribute = {nullptr, 0, Token::IDENTIFIER};
	section = {nullptr, 0, Token::IDENTIFIER};

	while (true) {
		if (!NextToken(text, &token)) return false;

		if (token.type == Token::END) break;

		if (token.type == Token::LEFT_BRACKET) {
			callback(attribute, section, {}, {}, EVENT_END_SECTION);

			token.type = Token::IDENTIFIER;
			if (!NextToken(text, &token, true)) return false;
			Token token2;
			if (!NextToken(text, &token2)) return false;

			if (token2.type == Token::IDENTIFIER) {
				attribute = token;
				section = token2;
				token.type = Token::RIGHT_BRACKET;
				if (!NextToken(text, &token, true)) return false;
			} else if (token2.type == Token::RIGHT_BRACKET) {
				attribute = {nullptr, 0, Token::IDENTIFIER};
				section = token;
			} else {
				printf("Unexpected token type. Found '%.*s'\n", token2.bytes, token2.text);
				return false;
			}

			callback(attribute, section, {}, {}, EVENT_START_SECTION);
		} else if (token.type == Token::IDENTIFIER) {
			Token name = token;

			token.type = Token::EQUALS;
			if (!NextToken(text, &token, true)) return false;
			if (!NextToken(text, &token)) return false;

			if (token.type != Token::IDENTIFIER
					&& token.type != Token::STRING
					&& token.type != Token::NUMERIC) {
				printf("Unexpected token type. Found '%.*s'\n", token.bytes, token.text);
				return false;
			}

			callback(attribute, section, name, token, EVENT_ATTRIBUTE);

			token.type = Token::SEMICOLON;
			if (!NextToken(text, &token, true)) return false;
		} else {
			printf("Unexpected token type. Found '%.*s'\n", token.bytes, token.text);
			return false;
		}
	}

	callback(attribute, section, {}, {}, EVENT_END_SECTION);

	return true;
}

FILE *output;
int actionCount;

bool CompareTokens(Token a, Token b) {
	if (a.bytes != b.bytes) return false;
	return 0 == memcmp(a.text, b.text, a.bytes);
}

bool CompareTokens(Token a, const char *string) {
	if (a.bytes != strlen(string)) return false;
	return 0 == memcmp(a.text, string, a.bytes);
}

void GenerateDeclarations(Token attribute, Token section, Token name, Token value, int event) {
	if (event != EVENT_START_SECTION) return;

	if (CompareTokens(attribute, "action")) {
		fprintf(output, "extern OSAction *%.*s;\n", section.bytes, section.text);
	}
}

void GenerateDefinitions(Token attribute, Token section, Token name, Token value, int event) {
	if (event == EVENT_START_SECTION) { 
		if (CompareTokens(attribute, "action")) {
			fprintf(output, "OSAction _%.*s = {\n", section.bytes, section.text);
		}
	}

	if (event == EVENT_ATTRIBUTE) { 
		if (CompareTokens(attribute, "action")) {
			fprintf(output, "\t.%.*s = %.*s,\n", name.bytes, name.text, value.bytes, value.text);

			if (value.type == Token::STRING) {
				fprintf(output, "\t.%.*sBytes = %d,\n", name.bytes, name.text, value.bytes - 2);
			}
		}
	}

	if (event == EVENT_END_SECTION) { 
		if (CompareTokens(attribute, "action")) {
			fprintf(output, "};\n\nOSAction *%.*s = &_%.*s;\n\n", section.bytes, section.text, section.bytes, section.text);
		}
	}
}

void GenerateActionList(Token attribute, Token section, Token name, Token value, int event) {
	if (event == EVENT_START_SECTION) { 
		if (CompareTokens(attribute, "action")) {
			actionCount++;
			fprintf(output, "\t%.*s,\n", section.bytes, section.text);
		}
	}
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: ./manifest_parser <input> <output>\n");
		return 1;
	}

	FILE *input = fopen(argv[1], "rb");
	fseek(input, 0, SEEK_END);
	size_t fileSize = ftell(input);
	fseek(input, 0, SEEK_SET);
	char *buffer = (char *) malloc(fileSize + 1);
	buffer[fileSize] = 0;
	fread(buffer, 1, fileSize, input);
	fclose(input);
	output = fopen(argv[2], "wb");

	fprintf(output, "// Header automatically generated by manifest_parser\n// Don't modify this file!\n\n");
	fprintf(output, "#ifndef OS_MANIFEST_HEADER\n#define OS_MANIFEST_HEADER\n\n");
	fprintf(output, "extern OSAction *_actions[];\nextern size_t _actionCount;\n\n");
	if (!ParseManifest(buffer, GenerateDeclarations)) return 1;
	fprintf(output, "\n#ifdef OS_MANIFEST_DEFINITIONS\n\n");
	if (!ParseManifest(buffer, GenerateDefinitions)) return 1;
	fprintf(output, "OSAction *_actions[] = {\n");
	if (!ParseManifest(buffer, GenerateActionList)) return 1;
	fprintf(output, "};\n\nsize_t _actionCount = %d;\n\n", actionCount);
	fprintf(output, "#endif\n\n#endif\n");

	fclose(output);
	free(buffer);

	return 0;
}
