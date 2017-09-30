#ifndef IMPLEMENTATION

enum VideoColorMode {
	// Little-endian.
	VIDEO_COLOR_24_RGB,
	VIDEO_COLOR_32_XRGB,
};

struct ModifiedScanline {
	// These values describe a range that encompasses all modified pixels on the scanline.
	// There may be unmodified pixels described by this.
	
	uint16_t minimumX,
		 maximumX;
};

struct Surface {
	// All Surfaces are currently VIDEO_COLOR_32_XRGB.
	// This reduces the amount of graphics function I have to write.

	bool Initialise(VMM *vmm /*the VMM to allocate the surface's memory from*/, 
			size_t resX, size_t resY,
			bool createDepthBuffer = false);

	// Copy the contents of the rectangular region sourceRegion from the source surface to this surface, 
	// where destinationPoint corresponds to where the top-left corner of the sourceRegion will be copied.
	// If avoidUnmodifiedRegions is set then regions that have not been marked as modified may not copied;
	// however, all of the modified regions will be definitely copied.
	// If depth is not SURFACE_COPY_WITHOUT_DEPTH_CHECKING, then for every pixel that is attempting to be copied,
	// it will check if the current value of depth is greater than the corresponding value in the depthBuffer;
	// if it is not, the pixel will not be drawn, otherwise the depth will be updated and the pixel will be drawn.
#define SURFACE_COPY_WITHOUT_DEPTH_CHECKING 0xFFFF
	void Copy(Surface &source, OSPoint destinationPoint, OSRectangle sourceRegion,
			bool avoidUnmodifiedRegions, uint16_t depth = SURFACE_COPY_WITHOUT_DEPTH_CHECKING); 

	// Alpha blend the contents of the source region from the source surface to the destination region on this surface.
	// The draw mode determines how the border is used.
	void Draw(Surface &source, OSRectangle destinationRegion, OSRectangle sourceRegion, 
			OSRectangle borderDimensions, OSDrawMode mode);

	// Fill a region of this surface with the specified color.
	void FillRectangle(OSRectangle region, OSColor color);

	// Mark a region of the surface as modified.
	void InvalidateScanline(uintptr_t y, uintptr_t from, uintptr_t to);
	void InvalidateRectangle(OSRectangle rectangle);

	// Mark the whole surface as unmodified.
	void ClearModifiedRegion();

	// The memory used by the surface.
	uint8_t *memory;
	bool memoryInKernelAddressSpace;

	uint8_t *linearBuffer; // The linear buffer of the surface. 
			       // Currently each pixel is 32-bit [A/X]RGB, little-endian.
	uint16_t *depthBuffer; // The depth of each pixel in the surface.

	size_t resX, resY; // Currently both must be < 65536.
	uintptr_t stride;  // Number of bytes from the start of a scanline to the start of the next.

	// A bitset of every modified scanline on the surface.
	uint8_t *modifiedScanlineBitset;

	// Contains the lowest and highest column that was modified on each scanline.
	// Only valid if the corresponding bit is set in the modified scanline bitset.
	ModifiedScanline *modifiedScanlines;

	// 'memory' consists of 3 sections:
	// 	  Contents				Size					Offset					Optional?
	// 	- Pixel data				resX * resY * 4				0					No
	// 	- Depth buffer				resX * resY * 4				resX * resY * 4				Yes
	// 	- Modified scanline bitset		(resY + 7) / 8				resX * resY * 8				Not yet
	// 	- Modified scanline information		resY * sizeof(ModifiedScanline)		resX * resY * 8 + (resY + 7) / 8	Not yet

	// Prevent multiple threads modifing the surface at the same time.
	Mutex mutex;
};

struct Graphics {
	void Initialise();

	void UpdateScreen();
	void UpdateScreen_VIDEO_COLOR_24_RGB();

	uint8_t *linearBuffer; 
	size_t resX, resY; 
	uintptr_t strideY, strideX;
	VideoColorMode colorMode;

	Surface frameBuffer;
	Pool surfacePool;
};

#ifdef ARCH_X86_64
struct VESAVideoModeInformation {
	// Useful fields marked with /**/
	uint16_t attributes;
	uint8_t windowA, windowB;
	uint16_t granularity;
	uint16_t windowSize;
	uint16_t segmentA, segmentB;
	uint32_t farWindowMove;
	uint16_t bytesPerScanline;
/**/	uint16_t widthPixels;
/**/	uint16_t heightPixels;
	uint8_t characterWidth;
	uint8_t characterHeight;
	uint8_t memoryPlanes;
/**/	uint8_t bitsPerPixel;
	uint8_t bankCount;
	uint8_t memoryModel;
	uint8_t bankSizeKB;
	uint8_t imagePages;
	uint8_t reserved;
	uint8_t redMaskSize;
	uint8_t redMaskPosition;
	uint8_t greenMaskSize;
	uint8_t greenMaskPosition;
	uint8_t blueMaskSize;
	uint8_t blueMaskPosition;
	uint8_t reservedMaskSize;
	uint8_t reservedMaskPosition;
	uint8_t directColorModeInfo;
/**/	uint32_t bufferPhysical;
	uint32_t offscreenMemoryPhysical;
	uint16_t offscreenMemorySizeKB;
/**/	uint16_t bytesPerScanlineLinear;
	uint8_t imagesBanked;
	uint8_t imagesLinear;
/**/	uint8_t linearRedMaskSize;
/**/	uint8_t linearRedMaskPosition;
/**/	uint8_t linearGreenMaskSize;
/**/	uint8_t linearGreenMaskPosition;
/**/	uint8_t linearBlueMaskSize;
/**/	uint8_t linearBlueMaskPosition;
	uint8_t linearReservedMaskSize;
	uint8_t linearReservedMaskPosition;
	uint32_t maximumPixelClock;
};

VESAVideoModeInformation *vesaMode = (VESAVideoModeInformation *) (LOW_MEMORY_MAP_START + 0x7000);
Graphics graphics;
#endif

#else

void Graphics::UpdateScreen_VIDEO_COLOR_24_RGB() {
	frameBuffer.mutex.Acquire();
	Defer(frameBuffer.mutex.Release());

	volatile uint8_t *pixel = (volatile uint8_t *) linearBuffer;
	volatile uint8_t *source = (volatile uint8_t *) frameBuffer.linearBuffer;

	for (uintptr_t y_ = 0; y_ < resY / 8; y_++) {
		if (frameBuffer.modifiedScanlineBitset[y_] == 0) {
			pixel += strideY * 8;
			source += frameBuffer.resX * 32;
			continue;
		}

		for (uintptr_t y = 0; y < 8; y++) {
			volatile uint8_t *scanlineStart = pixel;
			volatile uint8_t *sourceStart = source;

			if ((frameBuffer.modifiedScanlineBitset[y_] & (1 << y)) == 0) {
				pixel = scanlineStart + strideY;
				source += frameBuffer.resX * 4;
				continue;
			}

			ModifiedScanline *scanline = frameBuffer.modifiedScanlines + y + (y_ << 3);

			uintptr_t x = scanline->minimumX;
			pixel += 3 * x;
			source += 4 * x;

			uintptr_t total = scanline->maximumX - scanline->minimumX;

			if (simdSSSE3Support && total > 8) {
				size_t pixelGroups = total / 4 - 1;
				size_t pixels = pixelGroups * 4;
				SSSE3Framebuffer32To24Copy(pixel, source, pixelGroups);

				x += pixels;
				pixel += 3 * pixels;
				source += 4 * pixels;
			}

			while (x < scanline->maximumX) {
				pixel[0] = source[0];
				pixel[1] = source[1];
				pixel[2] = source[2];

				pixel += 3;
				source += 4;
				x++;
			}

			pixel = scanlineStart + strideY;
			source = sourceStart + frameBuffer.resX * 4;
		}
	}

	frameBuffer.ClearModifiedRegion();
}

void Graphics::UpdateScreen() {
	if (!linearBuffer) {
		return;
	}
		
	switch (colorMode) {
		case VIDEO_COLOR_24_RGB: {
			UpdateScreen_VIDEO_COLOR_24_RGB();
		} break;
					 
		default: {
			KernelPanic("Graphics::UpdateScreen - Unsupported color mode.\n");
		} break;
	}
}

void Graphics::Initialise() {
	linearBuffer = (uint8_t *) kernelVMM.Allocate(vesaMode->bytesPerScanlineLinear * vesaMode->heightPixels, vmmMapAll, vmmRegionPhysical, vesaMode->bufferPhysical, 0);
	resX = vesaMode->widthPixels;
	resY = vesaMode->heightPixels;
	strideX = vesaMode->bitsPerPixel >> 3;
	strideY = vesaMode->bytesPerScanlineLinear;
	colorMode = VIDEO_COLOR_24_RGB; // TODO Other color modes.

	surfacePool.Initialise(sizeof(Surface));

	frameBuffer.Initialise(&kernelVMM, resX, resY, true /*Create depth buffer for window manager*/);
	UpdateScreen();
}

bool Surface::Initialise(VMM *vmm, size_t _resX, size_t _resY, bool createDepthBuffer) {
	resX = _resX;
	resY = _resY;

	memoryInKernelAddressSpace = vmm == &kernelVMM;

	stride = resX * 4;

	// Check the surface is within our working size limits.
	if (!resX || !resY || resX >= 65536 || resY >= 65536) {
		return false;
	}

	// Allocate the memory needed by the surface.
	size_t memoryNeeded;
	if (createDepthBuffer) {
		memoryNeeded = resY * sizeof(ModifiedScanline) + resX * resY * 6 + (resY + 7) / 8;
		memory = (uint8_t *) vmm->Allocate(memoryNeeded);
		linearBuffer = memory;
		depthBuffer = (uint16_t *) (memory + resX * resY * 4);
		modifiedScanlines = (ModifiedScanline *) (depthBuffer + resX * resY);
		modifiedScanlineBitset = (uint8_t *) (modifiedScanlines + resY);
	} else {
		memoryNeeded = resY * sizeof(ModifiedScanline) + resX * resY * 4 + (resY + 7) / 8;
		memory = (uint8_t *) vmm->Allocate(memoryNeeded);
		linearBuffer = memory;
		depthBuffer = nullptr;
		modifiedScanlines = (ModifiedScanline *) (memory + resX * resY * 4);
		modifiedScanlineBitset = (uint8_t *) (modifiedScanlines + resY);
	}

	if (!memory) {
		return false;
	}

	KernelLog(LOG_VERBOSE, "Created surface using %dKB.\n", memoryNeeded / 1024);

	// We probably want to invalidate the whole surface when it is created.
	InvalidateRectangle(OSRectangle(0, resX, 0, resY));

	// We created the surface successfully!
	return true;
}

void Surface::ClearModifiedRegion() {
	mutex.AssertLocked();

	ZeroMemory(modifiedScanlineBitset, (resY + 7) / 8);
}

void Surface::InvalidateScanline(uintptr_t y, uintptr_t from, uintptr_t to) {
	mutex.AssertLocked();

	if ((modifiedScanlineBitset[y >> 3] & 1 << (y & 7))) {
		if (modifiedScanlines[y].minimumX > from)
			modifiedScanlines[y].minimumX = from;
		if (modifiedScanlines[y].maximumX < to)
			modifiedScanlines[y].maximumX = to;
	} else {
		modifiedScanlineBitset[y >> 3] |= 1 << (y & 7);
		modifiedScanlines[y].minimumX = from;
		modifiedScanlines[y].maximumX = to;
	}
}

void Surface::InvalidateRectangle(OSRectangle rectangle) {
	mutex.Acquire();
	Defer(mutex.Release());

	if (rectangle.bottom > resY) {
		rectangle.bottom = resY;
	}

	if (rectangle.right > resX) {
		rectangle.right = resX;
	}

	for (uintptr_t y = rectangle.top; y < rectangle.bottom; y++) {
		InvalidateScanline(y, rectangle.left, rectangle.right);
	}
}

void Surface::Copy(Surface &source, OSPoint destinationPoint, OSRectangle sourceRegion, bool avoidUnmodifiedRegions, uint16_t depth) {
	if (depth != SURFACE_COPY_WITHOUT_DEPTH_CHECKING) {
		if (!depthBuffer) {
			KernelPanic("Surface::Copy - Attempt to use depth checking on surface without depth buffer.\n");
		}
	}

	OSPoint sourceRegionDimensions = OSPoint(sourceRegion.right - sourceRegion.left,
						 sourceRegion.bottom - sourceRegion.top);

	OSRectangle destinationRegion = OSRectangle(destinationPoint.x,
						    destinationPoint.x + sourceRegionDimensions.x,
						    destinationPoint.y,
						    destinationPoint.y + sourceRegionDimensions.y);

	if (sourceRegion.top >= sourceRegion.bottom) return;
	if (sourceRegion.left >= sourceRegion.right) return;

	if (sourceRegion.bottom > source.resY) return;
	if (sourceRegion.right > source.resX) return;

	if (destinationRegion.bottom > resY) {
		sourceRegion.bottom -= destinationRegion.bottom - resY;
		sourceRegionDimensions.y -= destinationRegion.bottom - resY;
		destinationRegion.bottom = resY;
	}

	if (destinationRegion.right > resX) {
		sourceRegion.right -= destinationRegion.right - resX;
		sourceRegionDimensions.x -= destinationRegion.right - resX;
		destinationRegion.right = resX;
	}

	if (destinationRegion.top >= destinationRegion.bottom) return;
	if (destinationRegion.left >= destinationRegion.right) return;

	mutex.Acquire();
	Defer(mutex.Release());

	uint8_t *destinationPixel = linearBuffer + destinationRegion.top * stride + destinationRegion.left * 4;
	uint8_t *sourcePixel = source.linearBuffer + sourceRegion.top * source.stride + sourceRegion.left * 4;
	uint16_t *depthPixel = depthBuffer + destinationRegion.top * resX + destinationRegion.left;

	uintptr_t destinationY = destinationRegion.top;
	uintptr_t sourceY = sourceRegion.top;

	__m128i thisDepth = _mm_set1_epi16(depth);

	while (sourceY < sourceRegion.bottom) {
		uintptr_t offsetX = 0;
		size_t countX = sourceRegionDimensions.x;

		uint8_t *destinationStart = destinationPixel;
		uint8_t *sourceStart = sourcePixel;
		uint16_t *depthStart = depthPixel;

		if (avoidUnmodifiedRegions) {
			if (source.modifiedScanlineBitset[sourceY / 8] & (1 << (sourceY & 7))) {
				ModifiedScanline *scanline = source.modifiedScanlines + sourceY;

				uint16_t minimumX = scanline->minimumX;
				uint16_t maximumX = scanline->maximumX;

				if (maximumX < sourceRegion.left) {
					goto nextScanline;
				} else if (minimumX > sourceRegion.right) {
					goto nextScanline;
				}

				if (minimumX < sourceRegion.left) {
					minimumX = sourceRegion.left;
				}
				
				if (maximumX > sourceRegion.right) {
					maximumX = sourceRegion.right;
				}

				offsetX = minimumX - sourceRegion.left;
				countX = maximumX - minimumX;
			} else {
				goto nextScanline;
			}
		}

		InvalidateScanline(destinationY, destinationRegion.left + offsetX, destinationRegion.left + offsetX + countX);

		destinationPixel += offsetX * 4;
		sourcePixel += offsetX * 4;
		depthPixel += offsetX;

		if (depth == SURFACE_COPY_WITHOUT_DEPTH_CHECKING) {
			while (countX >= 4) {
				_mm_storeu_si128((__m128i *) destinationPixel, 
						_mm_loadu_si128((__m128i *) sourcePixel));
				destinationPixel += 16;
				sourcePixel += 16;
				countX -= 4;
			}

			while (countX >= 1) {
				*destinationPixel++ = *sourcePixel++;
				*destinationPixel++ = *sourcePixel++;
				*destinationPixel++ = *sourcePixel++;
				*destinationPixel++ = *sourcePixel++;

				countX -= 1;
			}
		} else {
#if 1
			while (countX >= 4) {
				__m128i oldDepth = _mm_loadl_epi64((__m128i *) depthPixel);
				__m128i maskDepth = _mm_cmplt_epi16(thisDepth, oldDepth);
				__m128i maskDepth128 = _mm_unpacklo_epi16(maskDepth, maskDepth);
				__m128i destinationValue = _mm_loadu_si128((__m128i *) destinationPixel);
				__m128i sourceValue = _mm_loadu_si128((__m128i *) sourcePixel);
				__m128i blendedValue = _mm_or_si128(_mm_and_si128(maskDepth128, destinationValue), _mm_andnot_si128(maskDepth128, sourceValue));
				__m128i blendedDepth = _mm_or_si128(_mm_and_si128(maskDepth, oldDepth), _mm_andnot_si128(maskDepth, thisDepth));
				_mm_storeu_si128((__m128i *) destinationPixel, blendedValue);
				_mm_storel_epi64((__m128i *) depthPixel, blendedDepth);

				destinationPixel += 16;
				sourcePixel += 16;
				depthPixel += 4;
				countX -= 4;
			}
#endif

			while (countX >= 1) {
				if (*depthPixel <= depth) {
					*depthPixel = depth;
					destinationPixel[0] = sourcePixel[0];
					destinationPixel[1] = sourcePixel[1];
					destinationPixel[2] = sourcePixel[2];
					destinationPixel[3] = sourcePixel[3];
				}

				destinationPixel += 4;
				sourcePixel += 4;
				depthPixel += 1;
				countX -= 1;
			}
		}

		nextScanline:
		destinationPixel = destinationStart + stride;
		sourcePixel = sourceStart + source.stride;
		depthPixel = depthStart + resX;
		destinationY++;
		sourceY++;
	}
}

void Surface::Draw(Surface &source, OSRectangle destinationRegion, OSRectangle sourceRegion, 
		OSRectangle borderDimensions, OSDrawMode mode) {
	// TODO Clean this code up here (same with Surface::Copy).
	
	if (sourceRegion.top >= sourceRegion.bottom) return;
	if (sourceRegion.left >= sourceRegion.right) return;

	if (sourceRegion.bottom > source.resY) return;
	if (sourceRegion.right > source.resX) return;

	if (destinationRegion.bottom > resY) destinationRegion.bottom = resY;
	if (destinationRegion.right > resX) destinationRegion.right = resX;

	if (destinationRegion.top >= destinationRegion.bottom) return;
	if (destinationRegion.left >= destinationRegion.right) return;

	if (borderDimensions.top + 1 >= sourceRegion.bottom) return;
	if (borderDimensions.left + 1 >= sourceRegion.right) return;

	mutex.Acquire();
	Defer(mutex.Release());

	uintptr_t rightBorderStart = destinationRegion.right - (sourceRegion.right - borderDimensions.right);
	uintptr_t bottomBorderStart = destinationRegion.bottom - (sourceRegion.bottom - borderDimensions.bottom);

	for (uintptr_t y = destinationRegion.top; y < destinationRegion.bottom; y++) {
		// TODO Other draw modes.
		(void) mode;

		uintptr_t sy = y - destinationRegion.top + sourceRegion.top;
		if (y >= bottomBorderStart) sy = y - destinationRegion.top - bottomBorderStart + borderDimensions.bottom;
		else if (sy > borderDimensions.top) sy = borderDimensions.top + 1;

		InvalidateScanline(y, destinationRegion.left, destinationRegion.right);

		for (uintptr_t x = destinationRegion.left; x < destinationRegion.right; x++) {
			uintptr_t sx = x - destinationRegion.left + sourceRegion.left;
			if (x >= rightBorderStart) sx = x - destinationRegion.left - rightBorderStart + borderDimensions.right;
			else if (sx > borderDimensions.left) sx = borderDimensions.left + 1;

			uint32_t *destinationPixel = (uint32_t *) (linearBuffer + x * 4 + y * stride);
			uint32_t *sourcePixel = (uint32_t *) (source.linearBuffer + sx * 4 + sy * source.stride);

			// TODO Alpha blending.
			*destinationPixel = *sourcePixel;
		}
	}
}

void Surface::FillRectangle(OSRectangle region, OSColor color) {
	if (region.left >= resX) return;
	if (region.top >= resY) return;

	mutex.Acquire();
	Defer(mutex.Release());

	if (region.right > resX) region.right = resX;
	if (region.bottom > resY) region.bottom = resY;

	uint8_t *destinationPixel = linearBuffer;

	destinationPixel += region.left * 4;
	destinationPixel += region.top * stride;

	uint8_t red = color.red;
	uint8_t green = color.green;
	uint8_t blue = color.blue;

	__m128i w = _mm_set_epi8(0, red, green, blue,
	 			 0, red, green, blue,
				 0, red, green, blue,
				 0, red, green, blue);

	for (uintptr_t y = region.top; y < region.bottom; y++) {
		uintptr_t remainingX = region.right - region.left;
		uint8_t *scanlineStart = destinationPixel;

		InvalidateScanline(y, region.left, region.right);

		while (remainingX >= 4) {
			_mm_storeu_si128((__m128i *) destinationPixel, w);
			destinationPixel += 16;
			remainingX -= 4;
		}

		while (remainingX) {
			destinationPixel[0] = color.blue;
			destinationPixel[1] = color.green;
			destinationPixel[2] = color.red;
			destinationPixel += 4;
			remainingX--;
		}

		destinationPixel = scanlineStart + 4 * resX;
	}
}

#endif
