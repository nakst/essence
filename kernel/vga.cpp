#ifndef IMPLEMENTATION

#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ  0x3C1

#define VGA_MISC_WRITE 0x3C2
#define VGA_MISC_READ  0x3CC

#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA  0x3C5

#define VGA_DAC_READ_INDEX  0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA        0x3C9

#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA  0x3CF

#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA  0x3D5

#define VGA_INSTAT_READ 0x3DA

uint8_t vgaMode18[] = {
	0xE3, 0x03, 0x01, 0x08, 0x00, 0x06, 0x5F, 0x4F,
	0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E, 0x00, 0x40,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEA, 0x0C,
	0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3, 0xFF, 0x00,
	0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F, 0xFF,
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0F, 0x00, 0x00,
};

#else

uint8_t *vgaAddress = (uint8_t *) LOW_MEMORY_MAP_START + 0xA0000;

void VGASetMode() {
	uint8_t *registers = vgaMode18;
	ProcessorOut8(VGA_MISC_WRITE, *registers++);
	for (int i = 0; i < 5; i++) { ProcessorOut8(VGA_SEQ_INDEX, i); ProcessorOut8(VGA_SEQ_DATA, *registers++); }
	ProcessorOut8(VGA_CRTC_INDEX, 0x03);
	ProcessorOut8(VGA_CRTC_DATA, ProcessorIn8(VGA_CRTC_DATA) | 0x80);
	ProcessorOut8(VGA_CRTC_INDEX, 0x11);
	ProcessorOut8(VGA_CRTC_DATA, ProcessorIn8(VGA_CRTC_DATA) & ~0x80);
	registers[0x03] |= 0x80;
	registers[0x11] &= ~0x80;
	for (int i = 0; i < 25; i++) { ProcessorOut8(VGA_CRTC_INDEX, i); ProcessorOut8(VGA_CRTC_DATA, *registers++); }
	for (int i = 0; i < 9; i++) { ProcessorOut8(VGA_GC_INDEX, i); ProcessorOut8(VGA_GC_DATA, *registers++); }
	for (int i = 0; i < 21; i++) { ProcessorIn8(VGA_INSTAT_READ); ProcessorOut8(VGA_AC_INDEX, i); ProcessorOut8(VGA_AC_WRITE, *registers++); }
	ProcessorIn8(VGA_INSTAT_READ);
	ProcessorOut8(VGA_AC_INDEX, 0x20);
}

void Graphics::VGAUpdateScreen() {
	for (int plane = 0; plane < 4; plane++) {
		volatile uint8_t *source = (volatile uint8_t *) frameBuffer.linearBuffer;

		ProcessorOut8(VGA_SEQ_INDEX, 2);
		ProcessorOut8(VGA_SEQ_DATA, 1 << plane);

		for (uintptr_t y_ = 0; y_ < resY / 8; y_++) {
			if (frameBuffer.modifiedScanlineBitset[y_] == 0) {
				source += frameBuffer.resX * 32;
				continue;
			}

			for (uintptr_t y = 0; y < 8; y++) {
				volatile uint8_t *sourceStart = source;

				if ((frameBuffer.modifiedScanlineBitset[y_] & (1 << y)) == 0) {
					source += frameBuffer.resX * 4;
					continue;
				}

				ModifiedScanline *scanline = frameBuffer.modifiedScanlines + y + (y_ << 3);

				uintptr_t x = scanline->minimumX & ~7;
				source += 4 * x;

				while (x < scanline->maximumX) {
					uint8_t v = 0;

					for (int i = 7; i >= 0; i--) {
						uint8_t p = plane == 3 ? 0 : source[plane];
						if (p >= 0x80) v |= 1 << i;
						source += 4;
					}

					vgaAddress[(y + y_ * 8) * 80 + (x >> 3)] = v;
					x += 8;
				}

				source = sourceStart + frameBuffer.resX * 4;
			}
		}
	}

	frameBuffer.ClearModifiedRegion();
}

#endif
