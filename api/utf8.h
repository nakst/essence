#ifndef __OS_UTF8
#define __OS_UTF8

#define UTF8_LENGTH_CHAR(character, value) { \
	char first = *character; \
 \
	if (!(first & 0x80)) \
		value = 1; \
	else if ((first & 0xE0) == 0xC0) \
		value = 2; \
	else if ((first & 0xF0) == 0xE0) \
		value = 3; \
  	else if ((first & 0xF8) == 0xF0) \
		value = 4; \
	else if ((first & 0xFC) == 0xF8) \
		value = 5; \
	else if ((first & 0xFE) == 0xFC) \
		value = 6; \
	else \
		value = 0; \
}

int utf8_length_char(char *character) {
	int value;
	UTF8_LENGTH_CHAR(character, value);
	return value;
}

int utf8_value(char *character) {
	int length;
	UTF8_LENGTH_CHAR(character, length);

	char first = *character;

	int value;

	if (length == 1)
		value = (int)first;
	else if (length == 2)
		value = (((int)first & 0x1F) << 6) | (((int)character[1]) & 0x3F);
	else if (length == 3)
		value = (((int)first & 0xF) << 12) | ((((int)character[1]) & 0x3F) << 6) | (((int)character[2]) & 0x3F);
	else if (length == 4)
		value = (((int)first & 0x7) << 18) | ((((int)character[1]) & 0x3F) << 12) | ((((int)character[2]) & 0x3F) << 6) |
		(((int)character[3]) & 0x3F);
	else if (length == 5)
		value = (((int)first & 0x3) << 24) | ((((int)character[1]) & 0x3F) << 18) | ((((int)character[2]) & 0x3F) << 12) |
		((((int)character[4]) & 0x3F) << 6) | (((int)character[5]) & 0x3F);
	else if (length == 6)
		value = (((int)first & 0x1) << 30) | ((((int)character[1]) & 0x3F) << 24) | ((((int)character[2]) & 0x3F) << 18) |
		((((int)character[4]) & 0x3F) << 12) | ((((int)character[5]) & 0x3F) << 6) | (((int)character[6]) & 0x3F);
	else
		value = 0; // Invalid code point

	return value;
}

char *utf8_advance(char *string) {
	int length;
	UTF8_LENGTH_CHAR(string, length);

	if (!length) // Invalid code point 
		return NULL;

	return string + length;
}

char *utf8_retreat(char *string) {
	// Keep going backwards until we find a non continuation character
	do string--;
	while (((*string) & 0xC0) == 0x80);
	return string;
}

int utf8_length(char *string, int max_bytes, int tab_size) {
	if (!string)
		return 0;
	if (!(*string))
		return 0;

	if (!max_bytes) return 0;

	int length = 0;
	char *limit = string + max_bytes;

	while ((max_bytes == -1 || string < limit) && *string) {
		if (!string) // Invalid code point
			return -1;

		if (utf8_value(string) == '\t')
			length = (length - (length % tab_size)) + tab_size;
		else length++;

		string = utf8_advance(string);
	}

	return length;
}

#endif
