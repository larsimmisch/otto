#ifndef NORITAKE_H_
#define NORITAKE_H_

#include <stdlib.h>
#include <string.h>
#include "fonts/fonts.h"

template <class SPI_, byte width_>
class _Noritake
{
public:

    static const byte width = width_;

    static void send(byte c, int period = 40) {
        SPI_::transfer(c, period);
    }

    static void send(const byte *data, int period) {
        for (; *data != 0; ++data) {
            SPI_::transfer(*data, period);
        }
    }

	static void send_rs(byte c, int period = 40) {
        // Set RS low
        SPI_::transfer((byte)0xf, 40);
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

	static byte character(byte x, byte y, byte c, const Glyph *font) {
        const Glyph *g = glyph_bsearch(c, font);

        if (!g)
        {
            return x;
        }
        byte sx = pgm_read_byte(&g->width);
        byte sy = pgm_read_byte(&g->height);

        const char *ps = (const char*)pgm_read_word(&g->bitmap);
        byte w = (sx - 1) / 8  + 1; // width in bytes

        area(x, y, x + sx - 1, y + sy - 1, 'h');
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

	static byte string_P(byte x1, byte y1, const byte *s, const Glyph *font) {
        for(byte i = 0;;++i)
        {
            byte v = pgm_read_byte(s + i);
            if (!v)
                break;

            x1 = character(x1, y1, v, font);
        }

        return x1;
    }

	static const Glyph* glyph_bsearch(byte key, const Glyph *base)
    {
        byte l, u, idx, v;
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
                byte width = pgm_read_byte(&g->width);
                w += width;
            }
        }

        return w;
    }
};

#endif
