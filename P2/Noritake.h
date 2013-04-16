#ifndef NORITAKE_H_
#define NORITAKE_H_

#include <stdlib.h>
#include <string.h>
#include "fonts/fonts.h"

template <class SPI_, byte width_>
class _Noritake
{
public:

    static void send(byte c, int period = 40) {
        SPI_::transfer(c, period);
    }

    static void send(const char *data, int period) {
        for (; *data != 0; ++data) {
            SPI_::transfer(*data, period);
        }
    }

	static void send_rs(byte c, int period = 40) {
        // Set RS low
        SPI_::transfer(0xf, 40);
        SPI_::transfer(c, period);
    }

    static void clear() {
        send_rs(0x01, 150);
    }

    static void on(bool cursor = true) {
        // Display on
        if (cursor)
            send_rs(0xd, 50);
        else
            send_rs(0xc, 50);
    }

    static void off() {
        send_rs(0x8, 50);
    }

    static void gcursor(byte x, byte y) {
        send_rs(0xf0, 40); // Graphical image
        send(x, 40);
        send(y, 40);
    }

	static void cursor(byte x, byte y) {
        switch (y)
        {
        case 0:
            send_rs(0x80 + x, 40);
            break;
        case 1:
            send_rs(0xc0 + x, 40);
            break;
        default:
            break;
        }
    }

	static void area(byte x1, byte y1, byte x2, byte y2, char command) {
        send_rs(0xf1, 40); // Graphical image
        send(x1, 40);
        send(y1, 40);
        send(x2, 40);
        send(y2, 40);
        send((byte)command, 500);
    }

	static void graphics(byte x1, byte y1, byte x2, byte y2, char direction,
				  byte count, const char *data) {
        area(x1, y1, x2, y2, direction);
        
        for (int i = 0; i < count; ++i)
        {
            send(data[i], 120);
        }
    }

	static void graphics_P(byte x1, byte y1, byte x2, byte y2, char direction,
					byte count, const char *data) {
        area(x1, y1, x2, y2, direction);
        
        for (int i = 0; i < count; ++i)
        {
            send(pgm_read_byte(data + i), 120);
        }
    }

	static byte character(byte x, byte y, char c, const Glyph *font) {
        const Glyph *g = glyph_bsearch(c, font);

        if (!g)
            return x;

        byte size = pgm_read_byte(&g->size);
        byte sx = (size & 0xf0) >> 4;
        byte sy = (size & 0x0f);

        const char *ps = (const char*)pgm_read_word(&g->bitmap);
        byte w = (sx / 8) + 1;

        area(x, y, x + w * 8 - 1, y + sy - 1, 'h');
        for (int i = 0; i < w * sy; ++i)
        {
            send(pgm_read_byte(ps + i), 120);
        }
        
        return x + sx;
    }

	static byte string(byte x1, byte y1, const char *s, const Glyph *font) {
        for(; *s; ++s)
        {
            x1 = character(x1, y1, *s, font); 
        }

        return x1;
    }

	static byte string_P(byte x1, byte y1, const char *s, const Glyph *font) {
        for(byte i = 0;;++i)
        {
            byte v = pgm_read_byte(s + i);
            if (!v)
                break;
		
            x1 = character(x1, y1, v, font); 
        }

        return x1;
    }

	// Volume bar
	static void volume(byte v) {
        const byte x_offset = 0;
        const byte y_offset = 11;
        const byte height = 4;
        // Each digit is 4 pixel space (including 1 pixel space)
        const byte digit_width = 4;
        const byte levelwidth = 3 * digit_width;
        // width of the bar, excluding borders
        const unsigned barwidth = width_ - levelwidth - 4;

        const unsigned vpos = (unsigned)v * height * barwidth / barwidth;

        // the borders are 2 pixel each
        // 0x80 is the left border
        byte bar[(barwidth + 4) / 8 + 1] = { 0x80 };
        // start after the left border
        byte bit = 0x20;
        byte k = 0;

        // right border
        bar[(barwidth - 2) / 8] = 0x40 >> ((barwidth + 2) % 8);

#if 0
        for (unsigned i = 0; i < vpos / height; ++i)
        {
            bar[k] |= bit;
            bit >>= 1;
            if (bit == 0)
            {
                bit = 0x80;
                ++k;
            }
        }
#endif

        const byte rem = height - (vpos % height);

        for (byte i = 0; i < rem; ++i)
        {
            graphics(x_offset, y_offset + i, x_offset + 8 * sizeof(bar)-1, 
                     y_offset + i, 'H',
                     sizeof(bar), (const char *)bar);
        }
      
#if 0  
        // extend the bar by one pixel
        bar[k] |= bit;
        
        for (byte i = rem; i < height; ++i)
        {
            graphics(x_offset, y_offset + i, x_offset + 8 * sizeof(bar)-1, 
                     y_offset + i, 'H',
                     sizeof(bar), (const char *)bar);
        }
#endif

        char buf[4] = { 0, 0, 0, 0 };
        itoa(v, buf, 10);

        // Clear level
        area(width_ - levelwidth, y_offset - 1, 
             width_ - 1, y_offset + 4, 'C');

        // Display the level
        string(width_ - levelwidth, y_offset - 1, buf, glyphs_4x6);
    }

	static const Glyph* glyph_bsearch(char key, const Glyph *base)
    { 
        byte l, u, idx;
        char v;
        l = 0;
        u = MAX_GLYPHS;
        while (l < u)
        {
            idx = (l + u) / 2;
            
            v = pgm_read_byte(&base[idx].code);
            if (key < v)
                u = idx;
            else if (key > v)
                l = idx + 1;
            else
                return base + idx;
        } 
        return 0;
    }

	static byte glyph_width(const char *s, const Glyph *font) {
        const Glyph *g;
        byte w = 0;
        
        for (; *s != 0; ++s)
        {
            g = glyph_bsearch(*s, font);
            if (g)
            {
                byte size = pgm_read_byte(&g->size);
                w += (size & 0xf0) >> 4;			
            }
        }

        return w;
    }
};

#endif
