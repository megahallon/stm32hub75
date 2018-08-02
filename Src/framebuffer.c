#include "framebuffer.h"

#define FRAMEBUFFER_BITLEN	FRAMEBUFFER_SHIFTLEN
#define FRAMEBUFFER_ROWLEN	(FRAMEBUFFER_BITLEN * FRAMEBUFFER_MAXBITDEPTH)
//Length of one framebuffer
#define FRAMEBUFFER_LEN		(FRAMEBUFFER_ROWLEN * MATRIX_PANEL_SCANROWS)

uint16_t framebuffers[FRAMEBUFFER_LEN * FRAMEBUFFER_BUFFERS] = { 0 };
unsigned int framebuffer_writebuffer = 0;
unsigned int framebuffer_displaybuffer = 0;

void framebuffer_init() {
	framebuffer_displaybuffer = 0;
	framebuffer_writebuffer = (framebuffer_displaybuffer + 1) % FRAMEBUFFER_BUFFERS;
}

void framebuffer_write(uint8_t x, uint8_t y, uint8_t rgb[3]) {
	uint8_t bits[] = {0, 1, 3, 4, 5, 8};
	uint8_t scanrow = y % MATRIX_PANEL_SCANROWS;
	uint8_t bus = (y / MATRIX_PANEL_SCANROWS) % MATRIX_PANEL_BUSES;
	unsigned int offset = (scanrow * FRAMEBUFFER_ROWLEN) + x;
	for (uint8_t channel = 0; channel < MATRIX_PANEL_CHANNELS; ++channel) {
		uint8_t value = rgb[channel];
		uint16_t *ptr = &framebuffers[(framebuffer_writebuffer * FRAMEBUFFER_LEN) + offset];
		uint16_t output = 1 << bits[channel + bus * MATRIX_PANEL_CHANNELS];
		for (uint16_t bit = (1 << (FRAMEBUFFER_MAXBITDEPTH - 1)); bit; bit >>= 1) {
			if (value & bit) {
				ptr[0] |= output;
			} else {
				ptr[0] &= ~output;
			}
			ptr += FRAMEBUFFER_BITLEN;
		}
	}
}

uint16_t *framebuffer_get() {
	return &framebuffers[framebuffer_displaybuffer * FRAMEBUFFER_LEN];
}

void framebuffer_swap() {
	framebuffer_displaybuffer = framebuffer_writebuffer;
	framebuffer_writebuffer = (framebuffer_displaybuffer + 1) % FRAMEBUFFER_BUFFERS;
}
