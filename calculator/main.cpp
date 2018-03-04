#include "../api/os.h"

#define OS_MANIFEST_DEFINITIONS
#include "../bin/os/calculator.manifest.h"

struct Instance {
	OSObject textbox;
};

struct Token {
	enum { ADD, SUBTRACT, MULTIPLY, DIVIDE, LEFT_BRACKET, RIGHT_BRACKET,
		PERCENTAGE_SIGN, NUMBER, EOL, ERROR, } type;

	union {
		double number;
	};
};

struct EvaluateResult {
	bool error;
	double value;
};

OSCallbackResponse Insert(OSObject object, OSMessage *message) {
	(void) object;

	char c = (char) (uintptr_t) message->context;
	Instance *instance = (Instance *) OSGetInstance(message->command.window);

	OSString oldText;
	OSGetText(instance->textbox, &oldText);

	if (oldText.bytes >= 1024) {
		// Too long.
		return OS_CALLBACK_NOT_HANDLED;
	}

	char newText[1024];
	OSCopyMemory(newText, oldText.buffer, oldText.bytes);
	newText[oldText.bytes] = c;
	OSSetText(instance->textbox, newText, oldText.bytes + 1);

	return OS_CALLBACK_HANDLED;
}

Token NextToken(char *&string, size_t &stringBytes) {
	while (stringBytes && (string[0] == ' ' || string[0] == '\t')) {
		string++;
		stringBytes--;
	}

	if (!stringBytes) {
		return {Token::EOL};
	}

	switch (string[0]) {
		case '+': { string++; stringBytes--; return {Token::ADD}; } break;
		case '-': { string++; stringBytes--; return {Token::SUBTRACT}; } break;
		case '*': { string++; stringBytes--; return {Token::MULTIPLY}; } break;
		case '/': { string++; stringBytes--; return {Token::DIVIDE}; } break;
		case '(': { string++; stringBytes--; return {Token::LEFT_BRACKET}; } break;
		case ')': { string++; stringBytes--; return {Token::RIGHT_BRACKET}; } break;
		case '%': { string++; stringBytes--; return {Token::PERCENTAGE_SIGN}; } break;

		default: {
			if ((string[0] >= '0' && string[0] <= '9') || string[0] == '.') {
				Token token = {Token::NUMBER};
				uint64_t n = 0, c = 1;
				bool f = false;
				// TODO Error or discard too many digits for integer and fractional parts respectively.

				while (stringBytes && ((string[0] >= '0' && string[0] <= '9') || (!f && string[0] == '.'))) {
					if (string[0] == '.') {
						f = true;
						goto next;
					}

					if (f) c *= 10;
					n *= 10;
					n += string[0] - '0';

					next:;
					string++;
					stringBytes--;
				}

				token.number = (double) n / (double) c;
				return token;
			} else {
				return {Token::ERROR};
			}
		} break;
	}
}

EvaluateResult Evaluate(char *&string, size_t &stringBytes, int precedence = 0) {
#define NEXT_TOKEN() NextToken(string, stringBytes)
#define EVALUATE(p) Evaluate(string, stringBytes, p)

	Token left = NEXT_TOKEN(), right;
	double number = 0;

	char *string2;
	size_t stringBytes2;

	switch (left.type) {
		case Token::NUMBER: {
			number = left.number;
		} break;

		case Token::LEFT_BRACKET: {
			EvaluateResult e = EVALUATE(0);
			if (e.error) goto error;
			number = e.value;

			Token rightBracket = NEXT_TOKEN();

			if (rightBracket.type != Token::RIGHT_BRACKET && rightBracket.type != Token::EOL) {
				goto error;
			}
		} break;

		case Token::SUBTRACT: {
			EvaluateResult e = EVALUATE(1000);
			if (e.error) goto error;
			number = -e.value;
		} break;

		default: {
			goto error;
		} break;
	}

	string2 = string;
	stringBytes2 = stringBytes;

	right = NEXT_TOKEN();

	while (right.type != Token::EOL && right.type != Token::ERROR) {
		switch (right.type) {
			case Token::ADD: {
				if (precedence < 3) {
					EvaluateResult e = EVALUATE(3);
					if (e.error) goto error;
					number += e.value;
				} else {
					string = string2;
					stringBytes = stringBytes2;
					goto done;
				}
			} break;

			case Token::SUBTRACT: {
				if (precedence < 3) {
					EvaluateResult e = EVALUATE(3);
					if (e.error) goto error;
					number -= e.value;
				} else {
					string = string2;
					stringBytes = stringBytes2;
					goto done;
				}
			} break;

			case Token::MULTIPLY: {
				if (precedence < 4) {
					EvaluateResult e = EVALUATE(4);
					if (e.error) goto error;
					number *= e.value;
				} else {
					string = string2;
					stringBytes = stringBytes2;
					goto done;
				}
			} break;

			case Token::DIVIDE: {
				if (precedence < 4) {
					EvaluateResult e = EVALUATE(4);
					if (e.error) goto error;
					if (e.value == 0) goto error;
					number /= e.value;
				} else {
					string = string2;
					stringBytes = stringBytes2;
					goto done;
				}
			} break;

			case Token::PERCENTAGE_SIGN: {
				number /= 100.0;
			} break;

			default: {
				string = string2;
				stringBytes = stringBytes2;
				goto done;
			} break;
		}

		string2 = string;
		stringBytes2 = stringBytes;
		right = NEXT_TOKEN();
	}

	done:;

	if (right.type == Token::ERROR) {
		goto error;
	}

	return {false, number};

	error:;
	return {true};

#undef NEXT_TOKEN
#undef EVALUATE
}

OSCallbackResponse Evaluate(OSObject object, OSMessage *message) {
	(void) object;

	Instance *instance = (Instance *) OSGetInstance(message->command.window);

	OSString expression;
	OSGetText(instance->textbox, &expression);

	char buffer[1024];
	EvaluateResult e = Evaluate(expression.buffer, expression.bytes);

	size_t length;

	if (e.error) {
		length = OSFormatString(buffer, 1024, "error");
	} else {
		length = OSFormatString(buffer, 1024, "%F", e.value);
	}

	OSSetText(instance->textbox, buffer, length);

	return OS_CALLBACK_HANDLED;
}

void ProgramEntry() {
	Instance *instance = (Instance *) OSHeapAllocate(sizeof(Instance), true);

	OSObject window = OSCreateWindow(mainWindow);
	OSSetInstance(window, instance);

	OSObject grid = OSCreateGrid(1, 2, OS_CREATE_GRID_NO_BORDER);
	OSObject keypad = OSCreateGrid(5, 4, 0);
	instance->textbox = OSCreateTextbox(18);

	OSSetRootGrid(window, grid);
	OSAddControl(grid, 0, 0, instance->textbox, OS_CELL_H_PUSH | OS_CELL_H_EXPAND);
	OSAddGrid(grid, 0, 1, keypad, 0);

	OSAddControl(keypad, 0, 0, OSCreateButton(insert7), 			OS_CELL_FILL);
	OSAddControl(keypad, 0, 1, OSCreateButton(insert4), 			OS_CELL_FILL);
	OSAddControl(keypad, 0, 2, OSCreateButton(insert1), 			OS_CELL_FILL);
	OSAddControl(keypad, 0, 3, OSCreateButton(insert0), 			OS_CELL_FILL);
	OSAddControl(keypad, 1, 0, OSCreateButton(insert8), 			OS_CELL_FILL);
	OSAddControl(keypad, 1, 1, OSCreateButton(insert5), 			OS_CELL_FILL);
	OSAddControl(keypad, 1, 2, OSCreateButton(insert2), 			OS_CELL_FILL);
	OSAddControl(keypad, 1, 3, OSCreateButton(insertFractionalSeparator), 	OS_CELL_FILL);
	OSAddControl(keypad, 2, 0, OSCreateButton(insert9), 			OS_CELL_FILL);
	OSAddControl(keypad, 2, 1, OSCreateButton(insert6), 			OS_CELL_FILL);
	OSAddControl(keypad, 2, 2, OSCreateButton(insert3), 			OS_CELL_FILL);
	OSAddControl(keypad, 2, 3, OSCreateButton(insertPercentageSign), 	OS_CELL_FILL);
	OSAddControl(keypad, 3, 0, OSCreateButton(insertDivide), 		OS_CELL_FILL);
	OSAddControl(keypad, 3, 1, OSCreateButton(insertMultiply), 		OS_CELL_FILL);
	OSAddControl(keypad, 3, 2, OSCreateButton(insertSubtract), 		OS_CELL_FILL);
	OSAddControl(keypad, 3, 3, OSCreateButton(insertAdd), 			OS_CELL_FILL);
	OSAddControl(keypad, 4, 0, OSCreateButton(insertLeftBracket), 		OS_CELL_FILL);
	OSAddControl(keypad, 4, 1, OSCreateButton(insertRightBracket), 		OS_CELL_FILL);
	OSAddControl(keypad, 4, 3, OSCreateButton(evaluate), 			OS_CELL_FILL);

	OSProcessMessages();
}
