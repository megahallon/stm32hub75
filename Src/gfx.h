#ifndef GFX
#define GFX

#include <stdint.h>
#include <stdbool.h>

void drawPixel(uint16_t x, uint16_t y, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
              uint16_t color);
void drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
              uint16_t color);
void drawFillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                  uint16_t color);
void drawChar(int16_t x, int16_t y, unsigned char c,
              uint16_t color, uint16_t bg, uint8_t size);
void drawString(const char* c);
void setCursor(int16_t x, int16_t y);
void setTextSize(uint8_t s);
void setTextColor(uint16_t c, uint16_t b);
void setTextWrap(bool w);

#endif
