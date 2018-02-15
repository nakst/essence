#include <ft2build.h>
#include FT_FREETYPE_H

typedef FT_Face Font;

static FT_Library freetypeLibrary;
static Font fontRegular, fontBold;

static bool fontRendererInitialised;

struct FontCacheEntry {
	uint8_t *data;
	int character, size;
	int width, height, xoff, yoff;
	int advanceWidth, glyphIndex;
	Font *font;
};

#define FONT_CACHE_SIZE 256
static FontCacheEntry fontCache[FONT_CACHE_SIZE];
static uintptr_t fontCachePosition;

static FontCacheEntry *LookupFontCacheEntry(int character, int size, Font &font) {
	// TODO Is there a faster way to do this?

	for (uintptr_t i = 0; i < FONT_CACHE_SIZE; i++) {
		FontCacheEntry *entry = fontCache + i;

		if (entry->character != character) continue;
		if (entry->size != size) continue;
		if (entry->font != &font) continue;

		return entry;
	}

	return nullptr;
}

static void OSFontRendererInitialise() {
	if (fontRendererInitialised) {
		return;
	}

	OSError error;

	OSNodeInformation nodeRegular;
	error = OSOpenNode(OSLiteral("/os/noto_sans/regular.ttf"), OS_OPEN_NODE_RESIZE_BLOCK | OS_OPEN_NODE_READ_ACCESS, &nodeRegular);
	if (error != OS_SUCCESS) return;
	void *loadedFontRegular = OSMapObject(nodeRegular.handle, 0, OS_SHARED_MEMORY_MAP_ALL, OS_MAP_OBJECT_READ_ONLY);
	if (!loadedFontRegular) return;

	OSNodeInformation nodeBold;
	error = OSOpenNode(OSLiteral("/os/noto_sans/bold.ttf"), OS_OPEN_NODE_RESIZE_BLOCK | OS_OPEN_NODE_READ_ACCESS, &nodeBold);
	if (error != OS_SUCCESS) return;
	void *loadedFontBold = OSMapObject(nodeBold.handle, 0, OS_SHARED_MEMORY_MAP_ALL, OS_MAP_OBJECT_READ_ONLY);
	if (!loadedFontBold) return;

	OSCloseHandle(nodeRegular.handle);
	OSCloseHandle(nodeBold.handle);

	{
		FT_Error error;

		error = FT_Init_FreeType(&freetypeLibrary);

		if (error) {
			OSPrint("Could not initialise freetype.\n");
			return;
		}

		error = FT_New_Memory_Face(freetypeLibrary, (uint8_t *) loadedFontRegular, nodeRegular.fileSize, 0, &fontRegular);
		error = FT_New_Memory_Face(freetypeLibrary, (uint8_t *) loadedFontBold, nodeBold.fileSize, 0, &fontBold);

		if (error) {
			OSPrint("Could not load the fonts into freetype.\n");
			return;
		}
	}

	fontRendererInitialised = true;
}

static int MeasureStringWidth(char *string, size_t stringLength, int size, Font &font) {
	char *stringEnd = string + stringLength;
	int totalWidth = 0;

	OSFontRendererInitialise();
	if (!fontRendererInitialised) return -1;

	FT_Set_Char_Size(font, 0, size * 64, 100, 100);

	while (string < stringEnd) {
		int character = utf8_value(string);

		int glyphIndex = FT_Get_Char_Index(font, character);
		FT_Load_Glyph(font, glyphIndex, FT_LOAD_DEFAULT);
		int advanceWidth = font->glyph->advance.x >> 6;

		totalWidth += advanceWidth;
		string = utf8_advance(string);
	}

	return totalWidth;
}

static void DrawCaret(OSPoint &outputPosition, OSRectangle &region, OSRectangle &invalidatedRegion, OSLinearBuffer &linearBuffer, int lineHeight, void *bitmap) {
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

		uint32_t *destination = (uint32_t *) ((uint8_t *) bitmap + 
				(oX) * 4 + 
				(oY) * linearBuffer.stride);
		*destination = 0xFF000000;
	}
}

static OSError DrawString(OSHandle surface, OSRectangle region, 
		OSString *string,
		unsigned alignment, uint32_t color, int32_t backgroundColor, uint32_t selectionColor,
		OSPoint coordinate, OSCaret *caret, uintptr_t caretIndex, uintptr_t caretIndex2, bool caretBlink,
		int size, Font &font) {
	bool actuallyDraw = caret == nullptr;

	OSFontRendererInitialise();
	if (!fontRendererInitialised) OSCrashProcess(OS_FATAL_ERROR_COULD_NOT_LOAD_FONT);

	FT_Set_Char_Size(font, 0, size * 64, 100, 100);

	int lineHeight = font->size->metrics.height >> 6; 
	int ascent = font->size->metrics.ascender >> 6; 

	char *stringEnd = string->buffer + string->bytes;

	OSLinearBuffer linearBuffer;
	void *bitmap = nullptr;
	if (surface != OS_INVALID_HANDLE) {
		OSGetLinearBuffer(surface, &linearBuffer);
		bitmap = OSMapObject(linearBuffer.handle, 0, linearBuffer.stride * linearBuffer.height, OS_MAP_OBJECT_READ_WRITE);
	}

	int totalWidth = MeasureStringWidth(string->buffer, string->bytes, size, font);

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
		caret->byte = caret->character = 0;
		return OS_SUCCESS;
	}

	char *buffer = string->buffer;

	while (buffer < stringEnd) {
		int character = utf8_value(buffer);

		int width, height, xoff, yoff;
		uint8_t *output = nullptr;
		int glyphIndex, advanceWidth;

		FontCacheEntry *cacheEntry = LookupFontCacheEntry(character, size, font);

		if (cacheEntry) {
			output = cacheEntry->data;
			width = cacheEntry->width;
			height = cacheEntry->height;
			xoff = cacheEntry->xoff;
			yoff = cacheEntry->yoff;
			glyphIndex = cacheEntry->glyphIndex;
			advanceWidth = cacheEntry->advanceWidth;
		} else {
			glyphIndex = FT_Get_Char_Index(font, character);
			FT_Load_Glyph(font, glyphIndex, FT_LOAD_DEFAULT);
			advanceWidth = font->glyph->advance.x >> 6;
		}

		bool selected = false;

		if (outputPosition.x + advanceWidth < region.left) {
			goto skipCharacter;
		} else if (outputPosition.x >= region.right) {
			break;
		}

		if (coordinate.x >= outputPosition.x && coordinate.x < outputPosition.x + advanceWidth / 2 && !actuallyDraw) {
			caret->character = characterIndex;
			caret->byte = buffer - string->buffer;
			return OS_SUCCESS;
		}

		if (coordinate.x >= outputPosition.x + advanceWidth / 2 && coordinate.x < outputPosition.x + advanceWidth && !actuallyDraw) {
			caret->character = characterIndex + 1;
			caret->byte = utf8_advance(buffer) - string->buffer;
			return OS_SUCCESS;
		}

		if (!actuallyDraw) {
			goto skipCharacter;
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

					uint32_t *destination = (uint32_t *) ((uint8_t *) bitmap + 
							(oX) * 4 + 
							(oY) * linearBuffer.stride);

					*destination = selectionColor;
				}
			}
		}

		if (!output) {
			FT_Render_Glyph(font->glyph, FT_RENDER_MODE_NORMAL);

			FT_Bitmap *bitmap = &font->glyph->bitmap;
			width = bitmap->width;
			height = bitmap->rows;
			xoff = font->glyph->bitmap_left;
			yoff = -font->glyph->bitmap_top;

			output = (uint8_t *) OSHeapAllocate(width * height, false);
			OSCopyMemory(output, bitmap->buffer, width * height);

			if (output) {
				FontCacheEntry *entry = fontCache + fontCachePosition;
				if (entry->data) OSHeapFree(entry->data);
				entry->data = output;
				entry->character = character;
				entry->width = width;
				entry->height = height;
				entry->xoff = xoff;
				entry->yoff = yoff;
				entry->advanceWidth = advanceWidth;
				entry->glyphIndex = glyphIndex;
				entry->size = size;
				entry->font = &font;
				fontCachePosition = (fontCachePosition + 1) % FONT_CACHE_SIZE;
			} else {
				goto skipCharacter;
			}
		}

		for (int y = 0; y < height; y++) {
			int oY = outputPosition.y + yoff + y;

			if (oY < region.top) continue;
			if (oY >= region.bottom) break;

			if (oY < 0) continue;
			if (oY >= (int) linearBuffer.height) break;

			if (oY < invalidatedRegion.top) invalidatedRegion.top = oY;
			if (oY > invalidatedRegion.bottom) invalidatedRegion.bottom = oY;

			for (int x = 0; x < width; x++) {
				int oX = outputPosition.x + xoff + x;
			
				if (oX < region.left) continue;
				if (oX >= region.right) break;

				if (oX < 0) continue;
				if (oX >= (int) linearBuffer.width) break;

				if (oX < invalidatedRegion.left) invalidatedRegion.left = oX;
				if (oX > invalidatedRegion.right) invalidatedRegion.right = oX;

				uint8_t pixel = output[x + y * width];
				uint32_t *destination = (uint32_t *) ((uint8_t *) bitmap + 
						(oX) * 4 + 
						(oY) * linearBuffer.stride);

				uint32_t sourcePixel = (pixel << 24) | color;

				if (pixel == 0xFF) {
					*destination = sourcePixel;
				} else if (pixel) {
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
			DrawCaret(outputPosition, region, invalidatedRegion, linearBuffer, lineHeight, bitmap);
		}

		outputPosition.x += advanceWidth;
		buffer = utf8_advance(buffer);
		characterIndex++;
	}

	if (characterIndex == caretIndex2 && caretIndex != (uintptr_t) -1 && !caretBlink) {
		DrawCaret(outputPosition, region, invalidatedRegion, linearBuffer, lineHeight, bitmap);
	}

	if (coordinate.x >= outputPosition.x && !actuallyDraw && characterIndex) {
		caret->character = characterIndex;
		caret->byte = buffer - string->buffer;
		return OS_SUCCESS;
	}

	if (surface != OS_INVALID_HANDLE) {
		OSInvalidateRectangle(surface, invalidatedRegion);
	}

	if (bitmap) {
		OSFree(bitmap);
		OSCloseHandle(linearBuffer.handle);
	}

	return actuallyDraw ? OS_SUCCESS : OS_ERROR_NO_CHARACTER_AT_COORDINATE;
}

#define FONT_SIZE (9)

OSError OSFindCharacterAtCoordinate(OSRectangle region, OSPoint coordinate, 
		OSString *string, unsigned alignment, OSCaret *caret) {
	return DrawString(OS_INVALID_HANDLE, region, 
			string,
			alignment, 0, 0, 0,
			coordinate, caret, -1, -1, false, FONT_SIZE, fontRegular);
}

OSError OSDrawString(OSHandle surface, OSRectangle region, 
		OSString *string, int fontSize,
		unsigned alignment, uint32_t color, int32_t backgroundColor, bool bold) {
	return DrawString(surface, region, 
			string,
			alignment, color, backgroundColor, 0,
			OSPoint(0, 0), nullptr, -1, -1, false, fontSize ? fontSize : FONT_SIZE, bold ? fontBold : fontRegular);
}
