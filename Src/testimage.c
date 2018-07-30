/*
 * testimage.c
 *
 *  Created on: 12 dec. 2014
 *      Author: Frans-Willem
 */

#include "framebuffer.h"
#include "stm32f1xx_hal.h"

#include <math.h>

void testimage_set(unsigned int x, unsigned int y, uint8_t r, uint8_t g, uint8_t b) {
	uint8_t rgb[3];
	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
	framebuffer_write(x, y, rgb);
}

void testimage_setb(unsigned int x, unsigned int y, uint8_t* rgb) {
	framebuffer_write(x, y, rgb);
}

#define MIN(a,b) (((a)<(b))?(a):(b))

static const int8_t sinetab[256] = {
     0,   2,   5,   8,  11,  15,  18,  21,
    24,  27,  30,  33,  36,  39,  42,  45,
    48,  51,  54,  56,  59,  62,  65,  67,
    70,  72,  75,  77,  80,  82,  85,  87,
    89,  91,  93,  96,  98, 100, 101, 103,
   105, 107, 108, 110, 111, 113, 114, 116,
   117, 118, 119, 120, 121, 122, 123, 123,
   124, 125, 125, 126, 126, 126, 126, 126,
   127, 126, 126, 126, 126, 126, 125, 125,
   124, 123, 123, 122, 121, 120, 119, 118,
   117, 116, 114, 113, 111, 110, 108, 107,
   105, 103, 101, 100,  98,  96,  93,  91,
    89,  87,  85,  82,  80,  77,  75,  72,
    70,  67,  65,  62,  59,  56,  54,  51,
    48,  45,  42,  39,  36,  33,  30,  27,
    24,  21,  18,  15,  11,   8,   5,   2,
     0,  -3,  -6,  -9, -12, -16, -19, -22,
   -25, -28, -31, -34, -37, -40, -43, -46,
   -49, -52, -55, -57, -60, -63, -66, -68,
   -71, -73, -76, -78, -81, -83, -86, -88,
   -90, -92, -94, -97, -99,-101,-102,-104,
  -106,-108,-109,-111,-112,-114,-115,-117,
  -118,-119,-120,-121,-122,-123,-124,-124,
  -125,-126,-126,-127,-127,-127,-127,-127,
  -128,-127,-127,-127,-127,-127,-126,-126,
  -125,-124,-124,-123,-122,-121,-120,-119,
  -118,-117,-115,-114,-112,-111,-109,-108,
  -106,-104,-102,-101, -99, -97, -94, -92,
   -90, -88, -86, -83, -81, -78, -76, -73,
   -71, -68, -66, -63, -60, -57, -55, -52,
   -49, -46, -43, -40, -37, -34, -31, -28,
   -25, -22, -19, -16, -12,  -9,  -6,  -3
};

#if 0
uint16_t colorcorr_table[1 + COLORCORR_GAMMA_COUNT][256];
unsigned int colorcorr_current;

float colorcorr_lum2duty(double lum) {
	if (lum>0.07999591993063804) {
		return pow(((lum+0.16)/1.16),3.0);
	} else {
		return lum/9.033;
	}
}

void colorcorr_init_table_direct(uint16_t *table) {
	unsigned int i;
	for (i=0; i<256; i++)
		table[i]=(i*((1<<FRAMEBUFFER_MAXBITDEPTH)-1))/255;
}

void colorcorr_init_table_lumgamma(uint16_t *table, double gamma) {
	unsigned int i,oi;
	for (i=0; i<256; i++) {
		double di = colorcorr_lum2duty(((double)i)/255.0);
		di = pow(di, gamma);
		oi = (unsigned int)(di * (double)((1<<FRAMEBUFFER_MAXBITDEPTH)-1));
		if (oi > (1<<FRAMEBUFFER_MAXBITDEPTH)-1)
			oi = (1<<FRAMEBUFFER_MAXBITDEPTH)-1;
		table[i] = oi;
	}
}

void colorcorr_init() {
	colorcorr_init_table_direct(colorcorr_table[0]);
	unsigned int i=0;
	for (i=0; i<=COLORCORR_GAMMA_COUNT; i++) {
		colorcorr_init_table_lumgamma(colorcorr_table[i+1],COLORCORR_GAMMA_MIN + (COLORCORR_GAMMA_STEP*i));
	}
#ifdef COLORCORR_GAMMA_DEFAULT
	colorcorr_current = 1+COLORCORR_GAMMA_DEFAULT;
#else
	colorcorr_current = 0;
#endif
}

uint16_t colorcorr_lookup(uint8_t v) {
	return colorcorr_table[colorcorr_current][v];
}

void colorcorr_select(unsigned int index) {
	if (index > COLORCORR_GAMMA_COUNT)
		index = COLORCORR_GAMMA_COUNT;
	colorcorr_current = index;
}
#endif

const float radius1 =16.3, radius2 =23.0, radius3 =40.8, radius4 =44.2,
            centerx1=16.1, centerx2=11.6, centerx3=23.4, centerx4= 4.1,
            centery1= 8.7, centery2= 6.5, centery3=14.0, centery4=-2.9;
float       angle1  = 0.0, angle2  = 0.0, angle3  = 0.0, angle4  = 0.0;
long        hueShift= 0;

#define FPS 15         // Maximum frames-per-second
uint32_t prevTime = 0; // For frame-to-frame interval timing

void ColorHSV(long hue, uint8_t sat, uint8_t val, char gflag, 
	uint8_t* _r, uint8_t* _g, uint8_t* _b) {

  uint8_t  r, g, b, lo;
  uint16_t s1, v1;

  // Hue
  hue %= 1536;             // -1535 to +1535
  if(hue < 0) hue += 1536; //     0 to +1535
  lo = hue & 255;          // Low byte  = primary/secondary color mix
  switch(hue >> 8) {       // High byte = sextant of colorwheel
    case 0 : r = 255     ; g =  lo     ; b =   0     ; break; // R to Y
    case 1 : r = 255 - lo; g = 255     ; b =   0     ; break; // Y to G
    case 2 : r =   0     ; g = 255     ; b =  lo     ; break; // G to C
    case 3 : r =   0     ; g = 255 - lo; b = 255     ; break; // C to B
    case 4 : r =  lo     ; g =   0     ; b = 255     ; break; // B to M
    default: r = 255     ; g =   0     ; b = 255 - lo; break; // M to R
  }

  // Saturation: add 1 so range is 1 to 256, allowig a quick shift operation
  // on the result rather than a costly divide, while the type upgrade to int
  // avoids repeated type conversions in both directions.
  s1 = sat + 1;
  r  = 255 - (((255 - r) * s1) >> 8);
  g  = 255 - (((255 - g) * s1) >> 8);
  b  = 255 - (((255 - b) * s1) >> 8);

  // Value (brightness) & 16-bit color reduction: similar to above, add 1
  // to allow shifts, and upgrade to int makes other conversions implicit.
  v1 = val + 1;
  *_r = (r * v1) >> 8;
  *_g = (g * v1) >> 8;
  *_b = (b * v1) >> 8;
}

void loop() {
  int           x1, x2, x3, x4, y1, y2, y3, y4, sx1, sx2, sx3, sx4;
  unsigned char x, y;
  long          value;
	uint8_t r, g, b;
	
  // To ensure that animation speed is similar on AVR & SAMD boards,
  // limit frame rate to FPS value (might go slower, but never faster).
  // This is preferable to delay() because the AVR is already plenty slow.
  uint32_t t;
  //while(((t = millis()) - prevTime) < (1000 / FPS));
	HAL_Delay(20);
  prevTime = t;

  sx1 = (int)(cos(angle1) * radius1 + centerx1);
  sx2 = (int)(cos(angle2) * radius2 + centerx2);
  sx3 = (int)(cos(angle3) * radius3 + centerx3);
  sx4 = (int)(cos(angle4) * radius4 + centerx4);
  y1  = (int)(sin(angle1) * radius1 + centery1);
  y2  = (int)(sin(angle2) * radius2 + centery2);
  y3  = (int)(sin(angle3) * radius3 + centery3);
  y4  = (int)(sin(angle4) * radius4 + centery4);

  for(y = 0; y < 32; y++) {
    x1 = sx1; x2 = sx2; x3 = sx3; x4 = sx4;
    for(x = 0; x < 32; x++) {
      value = hueShift
        + sinetab[MIN(255, (x1 * x1 + y1 * y1) >> 2)]
        + sinetab[MIN(255, (x2 * x2 + y2 * y2) >> 2)]
        + sinetab[MIN(255, (x3 * x3 + y3 * y3) >> 3)]
        + sinetab[MIN(255, (x4 * x4 + y4 * y4) >> 3)];
			ColorHSV(value * 3, 255, 255, 0, &r, &g, &b);
      testimage_set(x, y, r, g, b);
      x1--; x2--; x3--; x4--;
    }
    y1--; y2--; y3--; y4--;
  }

  angle1 += 0.03;
  angle2 -= 0.07;
  angle3 += 0.13;
  angle4 -= 0.15;
  hueShift += 2;
}

void testimage_init() {
	while (1) {
		loop();
	};
	

	unsigned int x, y;
	for (x = 0; x < MATRIX_WIDTH; ++x) {
		for (y = 0; y < MATRIX_HEIGHT; ++y) {
			if (x < 32)
				testimage_set(x, y, 0, y * 255 / 32, 0);
			/*
			if (y < 16) {
				unsigned int c = x * (sizeof(topcolors) / 3) / MATRIX_WIDTH;
				testimage_setb(x, y, &topcolors[c * 3]);
			} else {
				unsigned int c = x * (sizeof(barcolors) / 3) / MATRIX_WIDTH;
				testimage_setb(x, y, &barcolors[c * 3]);
			}
			*/
		}
	}
	framebuffer_swap();
}
