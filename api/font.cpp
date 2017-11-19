// TODO Thread safety.

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

static stbtt_fontinfo guiRegularFont;
static bool fontRendererInitialised;

struct FontCacheEntry {
	uint8_t *data;
	int character;
	int width, height, xoff, yoff;
};

#define FONT_CACHE_SIZE 256
static FontCacheEntry fontCache[FONT_CACHE_SIZE];
static uintptr_t fontCachePosition;

static void OSFontRendererInitialise() {
	if (fontRendererInitialised) {
		return;
	}

	OSHandle regularFontHandle = OSOpenNamedSharedMemory(OS_GUI_FONT_REGULAR, OSCStringLength(OS_GUI_FONT_REGULAR));

	if (regularFontHandle == OS_INVALID_HANDLE) {
		OSPrint("Could not get font handle.\n");
		return;
	}

	void *loadedFont = OSMapSharedMemory(regularFontHandle, 0, OS_SHARED_MEMORY_MAP_ALL);

	if (stbtt_InitFont(&guiRegularFont, (uint8_t *) loadedFont, 0)) {
		// The font was loaded.
	} else {
		return;
	}

	fontRendererInitialised = true;
}

static int MeasureStringWidth(char *string, size_t stringLength, float scale) {
	char *stringEnd = string + stringLength;
	int totalWidth = 0;

	OSFontRendererInitialise();
	if (!fontRendererInitialised) return -1;

	while (string < stringEnd) {
		int character = utf8_value(string);

		int advanceWidth, leftSideBearing;
		stbtt_GetCodepointHMetrics(&guiRegularFont, character, &advanceWidth, &leftSideBearing);
		advanceWidth    *= scale;
		leftSideBearing *= scale;

		totalWidth += advanceWidth;

		string = utf8_advance(string);
	}

	return totalWidth;
}

static float GetGUIFontScale() {
	OSFontRendererInitialise();
	if (!fontRendererInitialised) return OS_ERROR_COULD_NOT_LOAD_FONT;

#define FONT_SIZE (16.0f)
	float scale = stbtt_ScaleForPixelHeight(&guiRegularFont, FONT_SIZE);
	return scale;
}

static void DrawCaret(OSPoint &outputPosition, OSRectangle &region, OSRectangle &invalidatedRegion, OSLinearBuffer &linearBuffer, int lineHeight) {
	for (int y = 0; y < lineHeight; y++) {
		int oY = outputPosition.y - lineHeight + y + 4;

		if (oY < region.top) continue;
		if (oY >= region.bottom) break;

		if (oY < invalidatedRegion.top) invalidatedRegion.top = oY;
		if (oY > invalidatedRegion.bottom) invalidatedRegion.bottom = oY;

		int oX = outputPosition.x;

		if (oX < region.left) continue;
		if (oX >= region.right) break;

		if (oX < invalidatedRegion.left) invalidatedRegion.left = oX;
		if (oX > invalidatedRegion.right) invalidatedRegion.right = oX;

		uint32_t *destination = (uint32_t *) ((uint8_t *) linearBuffer.buffer + 
				(oX) * 4 + 
				(oY) * linearBuffer.stride);
		*destination = 0xFF000000;
	}
}

static OSError DrawString(OSHandle surface, OSRectangle region, 
		char *string, size_t stringLength,
		unsigned alignment, uint32_t color, int32_t backgroundColor, uint32_t selectionColor,
		OSPoint coordinate, uintptr_t *_characterIndex, uintptr_t caretIndex, uintptr_t caretIndex2, bool caretBlink) {
	bool actuallyDraw = _characterIndex == nullptr;

	OSFontRendererInitialise();
	if (!fontRendererInitialised) return OS_ERROR_COULD_NOT_LOAD_FONT;

	float scale = GetGUIFontScale();

	int ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&guiRegularFont, &ascent, &descent, &lineGap);
	ascent  *= scale;
	descent *= scale;
	lineGap *= scale;

	int lineHeight = ascent - descent + lineGap;

	char *stringEnd = string + stringLength;

	OSLinearBuffer linearBuffer;
	if (surface != OS_INVALID_HANDLE) {
		OSGetLinearBuffer(surface, &linearBuffer);
	}

	int totalWidth = MeasureStringWidth(string, stringLength, scale);

	OSPoint outputPosition;

	if (alignment & OS_DRAW_STRING_HALIGN_LEFT) {
		if (alignment & OS_DRAW_STRING_HALIGN_RIGHT) {
			// Centered text.
			outputPosition.x = (region.right - region.left) / 2 + region.left - totalWidth / 2;
		} else {
			// Left-justified text.
			outputPosition.x = region.left;
		}
	} else if (alignment & OS_DRAW_STRING_HALIGN_RIGHT) {
		// Right-justified text.
		outputPosition.x = region.right - totalWidth;
	} else	outputPosition.x = region.left;

	if (alignment & OS_DRAW_STRING_VALIGN_TOP) {
		if (alignment & OS_DRAW_STRING_VALIGN_BOTTOM) {
			// Centered text.
			outputPosition.y = (region.bottom - region.top) / 2 + region.top - lineHeight / 2 - 1;
		} else {
			// Top-justified text.
			outputPosition.y = region.top;
		}
	} else if (alignment & OS_DRAW_STRING_VALIGN_BOTTOM) {
		// Bottom-justified text.
		outputPosition.y = region.right - lineHeight;
	} else	outputPosition.y = region.top;

	outputPosition.y += ascent;

	OSRectangle invalidatedRegion = OSRectangle(outputPosition.x, outputPosition.x,
			outputPosition.y, outputPosition.y);

	uintptr_t characterIndex = 0;

	if (coordinate.x < outputPosition.x && !actuallyDraw) {
		*_characterIndex = characterIndex;
		return OS_SUCCESS;
	}

	while (string < stringEnd) {
		int character = utf8_value(string);

		int advanceWidth, leftSideBearing;
		stbtt_GetCodepointHMetrics(&guiRegularFont, character, &advanceWidth, &leftSideBearing);
		advanceWidth    *= scale;
		leftSideBearing *= scale;

		uint8_t *output = nullptr;

		bool selected = false;

		if (outputPosition.x + advanceWidth < region.left) {
			goto skipCharacter;
		} else if (outputPosition.x >= region.right) {
			break;
		}

		int ix0, iy0, ix1, iy1;
		stbtt_GetCodepointBitmapBox(&guiRegularFont, character, scale, scale, &ix0, &iy0, &ix1, &iy1);

		if (coordinate.x >= outputPosition.x && coordinate.x < outputPosition.x + advanceWidth / 2 && !actuallyDraw) {
			*_characterIndex = characterIndex;
			return OS_SUCCESS;
		}

		if (coordinate.x >= outputPosition.x + advanceWidth / 2 && coordinate.x < outputPosition.x + advanceWidth && !actuallyDraw) {
			*_characterIndex = characterIndex + 1;
			return OS_SUCCESS;
		}

		if (!actuallyDraw) {
			goto skipCharacter;
		}

		int width, height, xoff, yoff;

		for (uintptr_t i = 0; i < FONT_CACHE_SIZE; i++) {
			if (fontCache[i].character == character) {
				FontCacheEntry *entry = fontCache + i;
				output = entry->data;
				width = entry->width;
				height = entry->height;
				xoff = entry->xoff;
				yoff = entry->yoff;
				break;
			}
		}

		if (caretIndex != (uintptr_t) -1) {
			if (caretIndex < caretIndex2) {
				if (characterIndex >= caretIndex && characterIndex < caretIndex2) {
					selected = true;
				}
			} else {
				if (characterIndex >= caretIndex2 && characterIndex < caretIndex) {
					selected = true;
				}
			}
		}

		if (selected) {
			for (int y = 0; y < lineHeight; y++) {
				int oY = outputPosition.y - lineHeight + y + 4;

				if (oY < region.top) continue;
				if (oY >= region.bottom) break;

				if (oY < invalidatedRegion.top) invalidatedRegion.top = oY;
				if (oY > invalidatedRegion.bottom) invalidatedRegion.bottom = oY;

				for (int x = 0; x < advanceWidth + 1; x++) {
					int oX = outputPosition.x + x;

					if (oX < region.left) continue;
					if (oX >= region.right) break;

					if (oX < invalidatedRegion.left) invalidatedRegion.left = oX;
					if (oX > invalidatedRegion.right) invalidatedRegion.right = oX;

					uint32_t *destination = (uint32_t *) ((uint8_t *) linearBuffer.buffer + 
							(oX) * 4 + 
							(oY) * linearBuffer.stride);

					*destination = selectionColor;
				}
			}
		}

		if (!output) {
			output = stbtt_GetCodepointBitmap(&guiRegularFont, scale, scale, character, &width, &height, &xoff, &yoff);

			if (output) {
				FontCacheEntry *entry = fontCache + fontCachePosition;
				if (entry->data) OSHeapFree(entry->data);
				entry->data = output;
				entry->character = character;
				entry->width = width;
				entry->height = height;
				entry->xoff = xoff;
				entry->yoff = yoff;
				fontCachePosition = (fontCachePosition + 1) % FONT_CACHE_SIZE;

#if 0
				for (int x = 0; x < entry->width; x++) {
					for (int y = 0; y < entry->height; y++) {
						int32_t result = 0;
						int32_t totalWeight = 0;

						const int32_t weights[][3] = {
							{ 0, -1,  0},
							{-1, 16, -1},
							{ 0, -1,  0},
						};

						for (int i = -1; i <= 1; i++) {
							for (int j = -1; j <= 1; j++) {
								int x1 = x + i;
								int y1 = y + j;

								if (x1 >= 0 && x1 < entry->width && y1 >= 0 && y1 < entry->height) {
									int32_t source = entry->data[x1 + y1 * width];
									int32_t weight = weights[i + 1][j + 1];
									result += source * weight;
									totalWeight += weight > 0 ? weight : -weight;
								}
							}
						}

						if (totalWeight) {
							result /= totalWeight;
							
							if (result >= 0) {
								entry->data[x + y * width] = (uint8_t) result;
							} else {
								entry->data[x + y * width] = 0;
							}
						}
					}
				}
#endif
			} else {
				goto skipCharacter;
			}
		}

		for (int y = 0; y < height; y++) {
			int oY = outputPosition.y + iy0 + y;

			if (oY < region.top) continue;
			if (oY >= region.bottom) break;

			if (oY < invalidatedRegion.top) invalidatedRegion.top = oY;
			if (oY > invalidatedRegion.bottom) invalidatedRegion.bottom = oY;

			for (int x = 0; x < width; x++) {
				int oX = outputPosition.x + ix0 + x;
			
				if (oX < region.left) continue;
				if (oX >= region.right) break;

				if (oX < invalidatedRegion.left) invalidatedRegion.left = oX;
				if (oX > invalidatedRegion.right) invalidatedRegion.right = oX;

				uint8_t pixel = output[x + y * width];
				uint32_t *destination = (uint32_t *) ((uint8_t *) linearBuffer.buffer + 
						(oX) * 4 + 
						(oY) * linearBuffer.stride);

				uint32_t sourcePixel = (pixel << 24) | color;

				if (pixel == 0xFF) {
					*destination = sourcePixel;
				} else {
					uint32_t original;

					if (selected) {
						original = selectionColor;
					} else if (backgroundColor < 0) {
						original = *destination;
					} else {
						original = backgroundColor;
					}

					uint32_t modified = sourcePixel;

					uint32_t alpha1 = (modified & 0xFF000000) >> 24;
					uint32_t alpha2 = 255 - alpha1;
					uint32_t r2 = alpha2 * ((original & 0x000000FF) >> 0);
					uint32_t g2 = alpha2 * ((original & 0x0000FF00) >> 8);
					uint32_t b2 = alpha2 * ((original & 0x00FF0000) >> 16);
					uint32_t r1 = alpha1 * ((modified & 0x000000FF) >> 0);
					uint32_t g1 = alpha1 * ((modified & 0x0000FF00) >> 8);
					uint32_t b1 = alpha1 * ((modified & 0x00FF0000) >> 16);
					uint32_t result = 0xFF000000 | (0x00FF0000 & ((b1 + b2) << 8)) 
						| (0x0000FF00 & ((g1 + g2) << 0)) 
						| (0x000000FF & ((r1 + r2) >> 8));

					*destination = result;
				}
			}
		}

		skipCharacter:
		if (characterIndex == caretIndex2 && caretIndex != (uintptr_t) -1 && !caretBlink) {
			DrawCaret(outputPosition, region, invalidatedRegion, linearBuffer, lineHeight);
		}

		outputPosition.x += advanceWidth;
		string = utf8_advance(string);
		characterIndex++;
	}

	if (characterIndex == caretIndex2 && caretIndex != (uintptr_t) -1 && !caretBlink) {
		DrawCaret(outputPosition, region, invalidatedRegion, linearBuffer, lineHeight);
	}

	if (coordinate.x >= outputPosition.x && !actuallyDraw && characterIndex) {
		*_characterIndex = characterIndex;
		return OS_SUCCESS;
	}

	if (surface != OS_INVALID_HANDLE) {
		OSInvalidateRectangle(surface, invalidatedRegion);
	}

	return actuallyDraw ? OS_SUCCESS : OS_ERROR_NO_CHARACTER_AT_COORDINATE;
}

OSError OSFindCharacterAtCoordinate(OSRectangle region, OSPoint coordinate, 
		char *string, size_t stringLength, 
		unsigned alignment, uintptr_t *characterIndex) {
	return DrawString(OS_INVALID_HANDLE, region, 
			string, stringLength,
			alignment, 0, 0, 0,
			coordinate, characterIndex, -1, -1, false);
}

OSError OSDrawString(OSHandle surface, OSRectangle region, 
		char *string, size_t stringLength,
		unsigned alignment, uint32_t color, int32_t backgroundColor) {
	return DrawString(surface, region, 
			string, stringLength,
			alignment, color, backgroundColor, 0,
			OSPoint(0, 0), nullptr, -1, -1, false);
}
