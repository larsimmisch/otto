#ifndef FONTS_H
#define FONTS_H

#include <avr/pgmspace.h>

#define MAX_GLYPHS 71

struct Glyph
{
    unsigned char code;
    unsigned char size;
    const char *bitmap;
};

const extern struct Glyph glyphs_4x6[] PROGMEM;
const extern struct Glyph glyphs_medium[] PROGMEM;

#endif // FONTS_H
