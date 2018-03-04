#include <stdio.h>
#include <stdint.h>
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
		IDENTIFIER, NUMERIC, STRING, PERCENT,
		END,
	} type;
};

#define EVENT_END_SECTION (0)
#define EVENT_START_SECTION (1)
#define EVENT_ATTRIBUTE (2)

typedef void (*ParseCallback)(Token attribute, Token section, Token variable, Token value, int event);

static void RemoveEscapeSequencesFromString(Token *token) {
	for (uintptr_t i = 0; i < token->bytes; i++) {
		if (token->text[i] == '\\') {
			if (token->text[i + 1] == '$') {
				token->text[i] = 0x11;
				token->text[i + 1] = 0x12;
			}
		}
	}
}

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
	} else if (c == '%') {
		token->type = Token::PERCENT;
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
	} else if (c == '\'') {
		token->type = Token::STRING;
		buffer = token->text + token->bytes;
		c = utf8_value(buffer);

		while (c != '\'' && c) {
			token->bytes += utf8_length_char(buffer);
			buffer = token->text + token->bytes;
			c = utf8_value(buffer);
		}

		if (!c) {
			printf("Unexpected end of file in string\n");
			return false;
		}

		token->bytes++;
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
		RemoveEscapeSequencesFromString(token);
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

static bool CompareTokens(Token a, Token b) {
	if (a.bytes != b.bytes) return false;
	return 0 == memcmp(a.text, b.text, a.bytes);
}

static bool CompareTokens(Token a, const char *string) {
	if (a.bytes != strlen(string)) return false;
	return 0 == memcmp(a.text, string, a.bytes);
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
int commandCount;

struct CommandProperty {
	Token name;
	Token value;
};

CommandProperty properties[256];
int propertyCount;

bool FindProperty(const char *name, Token *value) {
	int x = strlen(name);

	for (int i = 0; i < propertyCount; i++) {
		if (x == properties[i].name.bytes && 0 == memcmp(name, properties[i].name.text, x)) {
			*value = properties[i].value;
			return true;
		}
	}

	return false;
}

void GenerateDeclarations(Token attribute, Token section, Token name, Token value, int event) {
	if (event != EVENT_START_SECTION) return;

	if (CompareTokens(attribute, "command")) {
		fprintf(output, "extern OSCommand *%.*s;\n", section.bytes, section.text);
	}
}

void GenerateDefinitions(Token attribute, Token section, Token name, Token value, int event) {
	if (event == EVENT_START_SECTION) { 
		propertyCount = 0;

		if (CompareTokens(attribute, "command")) {
			commandCount++;
		} else if (CompareTokens(attribute, "window")) {
		}
	}

	if (event == EVENT_ATTRIBUTE) { 
		if (CompareTokens(attribute, "command") || CompareTokens(attribute, "window")) {
			properties[propertyCount++] = { name, value };
		}
	}

	if (event == EVENT_END_SECTION) { 
		if (CompareTokens(attribute, "command")) {
			Token value;

			if (FindProperty("callback", &value)) {
// typedef OSCallbackResponse (*OSCallbackFunction)(OSObject object, OSMessage *);
				fprintf(output, "OSCallbackResponse %.*s(OSObject object, OSMessage *message);\n\n", value.bytes, value.text);
			}

			fprintf(output, "OSCommand _%.*s = {\n\t.identifier = %d,\n", section.bytes, section.text, commandCount - 1);

			if (FindProperty("label", &value)) {
				fprintf(output, "\t.label = (char *) %.*s,\n", value.bytes, value.text);
				fprintf(output, "\t.labelBytes = %d,\n", value.bytes - 2);
			} else {
				fprintf(output, "\t.label = (char *) \"\",\n");
				fprintf(output, "\t.labelBytes = 0,\n");
			}

			if (FindProperty("shortcut", &value)) {
				fprintf(output, "\t.shortcut = (char *) %.*s,\n", value.bytes, value.text);
				fprintf(output, "\t.shortcutBytes = %d,\n", value.bytes - 2);
			} else {
				fprintf(output, "\t.shortcut = (char *) \"\",\n");
				fprintf(output, "\t.shortcutBytes = 0,\n");
			}

			if (FindProperty("checkable", &value)) {
				fprintf(output, "\t.checkable = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.checkable = false,\n");
			}

			if (FindProperty("defaultCheck", &value)) {
				fprintf(output, "\t.defaultCheck = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.defaultCheck = false,\n");
			}

			if (FindProperty("defaultDisabled", &value)) {
				fprintf(output, "\t.defaultDisabled = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.defaultDisabled = %s,\n", FindProperty("callback", &value) ? "false" : "true");
			}

			fprintf(output, "\t.callback = { ");

			if (FindProperty("callback", &value)) {
				fprintf(output, "%.*s, ", value.bytes, value.text);
			} else {
				fprintf(output, "nullptr, ");
			}

			if (FindProperty("callbackArgument", &value)) {
				fprintf(output, "(void *) %.*s },\n", value.bytes, value.text);
			} else {
				fprintf(output, "nullptr },\n");
			}

			fprintf(output, "};\n\nOSCommand *%.*s = &_%.*s;\n\n", section.bytes, section.text, section.bytes, section.text);
		} else if (CompareTokens(attribute, "window")) {
			fprintf(output, "OSWindowSpecification _%.*s = {\n", section.bytes, section.text);

typedef struct OSWindowSpecification {
	unsigned width, height;
	unsigned minimumWidth, minimumHeight;

	unsigned flags;

	char *title;
	size_t titleBytes;
} OSWindowSpecification;

			if (FindProperty("width", &value)) {
				fprintf(output, "\t.width = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.width = 800,\n");
			}

			if (FindProperty("height", &value)) {
				fprintf(output, "\t.height = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.height = 600,\n");
			}

			if (FindProperty("minimumWidth", &value)) {
				fprintf(output, "\t.minimumWidth = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.minimumWidth = 200,\n");
			}

			if (FindProperty("minimumHeight", &value)) {
				fprintf(output, "\t.minimumHeight = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.minimumHeight = 160,\n");
			}

			if (FindProperty("flags", &value)) {
				fprintf(output, "\t.flags = %.*s,\n", value.bytes, value.text);
			} else {
				fprintf(output, "\t.flags = 0,\n");
			}

			if (FindProperty("title", &value)) {
				fprintf(output, "\t.title = (char *) %.*s,\n", value.bytes, value.text);
				fprintf(output, "\t.titleBytes = %d,\n", value.bytes - 2);
			} else {
				fprintf(output, "\t.title = \"New Window\",\n");
				fprintf(output, "\t.titleBytes = 10,\n");
			}

			fprintf(output, "};\n\nOSWindowSpecification *%.*s = &_%.*s;\n\n", section.bytes, section.text, section.bytes, section.text);
		}
	}
}

void GenerateActionList(Token attribute, Token section, Token name, Token value, int event) {
	if (event == EVENT_START_SECTION) { 
		if (CompareTokens(attribute, "command")) {
			fprintf(output, "\t&_%.*s,\n", section.bytes, section.text);
		}
	}
}

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Usage: ./manifest_parser <input> <output>\n");
		return 1;
	}

	size_t fileSize = 0;

	FILE *standard = fopen("api/standard.manifest", "rb");
	fseek(standard, 0, SEEK_END);
	fileSize += ftell(standard);
	fseek(standard, 0, SEEK_SET);

	FILE *input = fopen(argv[1], "rb");
	fseek(input, 0, SEEK_END);
	fileSize += ftell(input);
	fseek(input, 0, SEEK_SET);

	char *buffer = (char *) malloc(fileSize + 1);
	buffer[fileSize] = 0;

	size_t s = fread(buffer, 1, fileSize, standard);
	fread(buffer + s, 1, fileSize, input);

	fclose(standard);
	fclose(input);

	output = fopen(argv[2], "wb");

	fprintf(output, "// Header automatically generated by manifest_parser.\n// Don't modify this file!\n\n");
	fprintf(output, "#ifndef OS_MANIFEST_HEADER\n#define OS_MANIFEST_HEADER\n\n");
	fprintf(output, "extern OSCommand *_commands[];\nextern size_t _commandCount;\n\n");
	if (!ParseManifest(buffer, GenerateDeclarations)) return 1;
	fprintf(output, "\n#ifdef OS_MANIFEST_DEFINITIONS\n\n");
	if (!ParseManifest(buffer, GenerateDefinitions)) return 1;
	fprintf(output, "OSCommand *_commands[] = {\n");
	if (!ParseManifest(buffer, GenerateActionList)) return 1;
	fprintf(output, "};\n\nsize_t _commandCount = %d;\n\n", commandCount);
	fprintf(output, "#endif\n\n#endif\n");

	fclose(output);
	free(buffer);

	return 0;
}
