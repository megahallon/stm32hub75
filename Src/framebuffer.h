/*
 * framebuffer.h
 *
 *  Created on: 11 dec. 2014
 *      Author: Frans-Willem
 */

#ifndef FRAMEBUFFER_H_
#define FRAMEBUFFER_H_
#include <stdint.h>
#include "config.h"

#define CLK 1
 
//Number of bits to clock out each time
#define FRAMEBUFFER_BITSPERLINE	(MATRIX_PANEL_WIDTH * MATRIX_PANELSW * MATRIX_PANELSH)
#define FRAMEBUFFER_SHIFTLEN	(FRAMEBUFFER_BITSPERLINE * CLK)
#define FRAMEBUFFER_CLOCK	(1<<15)

void framebuffer_init(void);
void framebuffer_write(uint8_t x, uint8_t y, uint8_t rgb[3]);
uint16_t *framebuffer_get(void);
void framebuffer_swap(void);

#endif /* FRAMEBUFFER_H_ */
