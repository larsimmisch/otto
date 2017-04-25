#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
from optparse import OptionParser
from PIL import Image

# Simple test:
# >>> from fonts import *
# >>> a = Glyph('a', 'medium')
# a.show()

chars = '\x010123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz[]():., °'

struct_glyph = """struct Glyph
{
    unsigned char code;
    unsigned char size;
    const char *bitmap;
};

"""

char_map = {
    '°': 176
}

def convert_byte(values):
    value = 0
    bit = 128

    for v in values:
        # Black becomes white
        if v == 0:
            value = value | bit
        bit = bit >> 1

    return value

class Fontfile:
    def __init__(self, name, fname, offset = 0):
        self.name = name
        self.image = Image.open(fname).convert('1')
        self.offset = offset
        self.compute_offsets()

    def compute_offsets(self):
        # glyphs are separated by one or more
        # black pixels in the last line
        state = 0
        last = None
        y = self.image.size[1] - 1
        offsets = []

        pixels = self.image.load()
    
        for i in range(self.image.size[0]):
            if pixels[i, y] != state:
                if pixels[i, y] != 0 and state == 0:
                    last = i
                else:
                    offsets.append((last, i))
                state = pixels[i, y]

        self.offsets = offsets

    def get_glyph(self, c):
        i = char_map.get(c, ord(c))
        
        nospace = self.image.crop((self.offsets[i][0], self.offset,
                                   self.offsets[i][1], self.image.size[1] - 1))
        glyph = Image.new('1', (nospace.size[0] + 1, nospace.size[1]), 255)
        glyph.paste(nospace, (0, 0))
        return glyph
        
class Glyph:
    def __init__(self, c, fontfile, name):
        self.char = c
        self.vdata = None
        self.image = None
        self.size = None
        self.name = name
        self.load(fontfile)
        self.update_vdata()

    def load(self, fontfile):
        self.image = fontfile.get_glyph(self.char)
        self.size = self.image.size

    def update_vdata(self):
        self.vdata = []
        pixels = self.image.load()
        # print 'Glyph %s: %s (%s)' % (self.char, self.size, self.image.size)
        for o in range(((self.size[0] - 1) // 8) + 1):
            r = []
            upper = (o + 1) * 8
            if o == self.size[0] // 8:
                upper = min(self.size[0], upper)
            # print '  %d, %d' % (o, upper)
            for p in range(self.size[1]):
                l = [pixels[i, p] for i in range(o * 8, upper)]
                # print '    %s' % (l)
                r.append(convert_byte(l))
            self.vdata.append(r)

    def c_bitmap_literal(self):
        s = 'const char bm_%s_%02x[] PROGMEM = "' % (self.name, ord(self.char))
        indent = len(s) -1
        for c in self.vdata[0]:
            s = s + r'\x%02x' % c
        for r in self.vdata[1:]:
            s = s + '"\n' + indent * ' ' + '"'
            for c in r:
                s = s + r'\x%02x' % c
        return s + '";\n'

    def c_literal(self, indent = 4):
        s = indent * ' ' + "{ '" + self.char + "', "
        s = s + '0x%02x, ' % ((self.size[0] << 4) + self.size[1])
        s = s + 'bm_%s_%02x, },\n' % (self.name, ord(self.char))
        return s

    def show(self):
        self.image.show()

def extract_glyphs():
    glyphs = {}
    fonts = [Fontfile('medium', 'medium.2.font.bmp', 7),
             Fontfile('huge', 'huge.2.font.bmp')]
    for f in fonts:
        d = {}
        for c in chars:
            d[c] = Glyph(c, f, f.name)
        glyphs[f.name] = d
    return glyphs

glyphs = extract_glyphs()

def bw(v):
    if v:
        return 0
    return 255

class Noritake:
    def __init__(self):
        self.image = Image.new('1', (120, 16), 255)

    def put_glyph(self, pos, c, font):
        g = glyphs[font][c]
        p = pos
        for r in g.vdata:
            self.graphics_h(p, r)
            p = (p[0] + 8, p[1])
        return (pos[0] + g.size[0], pos[1])

    def put_string(self, pos, s, font):
        for c in s:
            pos = self.put_glyph(pos, c, font)
        return pos

    def graphics(self, area, fmt, data):
        if fmt not in 'hHvV':
            raise ValueError("format must be one 'h', 'H', 'v' or 'V'")

        if area[2] >= area[0]:
            raise ValueError("x0 >= x1")

        if area[1] >= area[3]:
            raise ValueError("y0 >= y1")

        self.__class__.__dict__['grapics_' + fmt]((area[0], area[1]), data)

    def put_vertical(self, pos, v):
        bit = 128
        try:
            for i in range(8):
                self.image.putpixel((pos[0], pos[1] + i), bw(v & bit))
                bit = bit >> 1
        except:
           print(pos)
           raise

    def put_horizontal(self, pos, v):
        bit = 128
        try:
            for i in range(8):
                self.image.putpixel((pos[0] + i, pos[1]), bw(v & bit))
                bit = bit >> 1
        except:
            print(pos)
            raise

    def graphics_V(self, pos, data):
        # Vertical data with horizontal cursor
        for d in data:
            self.put_vertical(pos, d)
            pos = (pos[0] + 1, pos[1])
            
        return pos
                  
    def graphics_v(self, pos, data):
        # Vertical data with vertical cursor
        for d in data:
            self.put_vertical(pos, d)
            pos = (pos[0], pos[1] + 8)

        return pos

    def graphics_H(self, pos, data):
        # Horizontal data with horizontal cursor
        for d in data:
            self.put_horizontal(pos, d)
            pos = (pos[0] + 8, pos[1])

        return pos

    def graphics_h(self, pos, data):
        # Horizontal data with vertical cursor
        for d in data:
            self.put_horizontal(pos, d)
            pos = (pos[0], pos[1] + 1)

        return pos
        
    def show(self):
        self.image.show()

def generate_fonts(fname, iname):
    with open(iname, "w") as f:
        guard = iname.replace('.', '_').upper()
        f.write('#ifndef %s\n' % guard)
        f.write('#define %s\n\n' % guard)
        f.write('#include <avr/pgmspace.h>\n\n')
        f.write('#define MAX_GLYPHS %d\n\n' % len(chars))        
        f.write(struct_glyph)

        for n, gl in glyphs.items():
            f.write("const extern struct Glyph glyphs_%s[] PROGMEM;\n" % n)

        f.write('\n#endif // %s\n' % guard)


    with open(fname, "w") as f:
        f.write('#include "%s"\n\n' % iname)        

        for n, gl in glyphs.items():
            keys = list(gl.keys())
            keys.sort()

            for c in keys:
                f.write(gl[c].c_bitmap_literal())

            f.write("\nconst struct Glyph glyphs_%s[] PROGMEM = {\n" % n)
            for c in keys:
                f.write(gl[c].c_literal())
            f.write("};\n\n")

if __name__ == '__main__':

    parser = OptionParser(usage='usage: %prog [options] [<filename>]')
    parser.add_option('-f', '--font', type='string', help='File that will '\
                      'contain the font data as C structure definitions.')
    parser.add_option('-i', '--include', type='string', help='Header file ' \
                      'that will contain the font data as C structure ' \
                      'declarations.')
    parser.add_option('-o', '--out', type='string', help='File that will '\
                      'contain a bitmap of the simulated display.')
    parser.add_option('-1', '--line1', type='string',
                      help='First (large) item on the display')
    parser.add_option('-2', '--line2', type='string',
                      help='Second (smaller) item on the display')

    options, args = parser.parse_args()

    if options.font and options.include:
        generate_fonts(options.font, options.include)

    if options.line1 or options.line2:
        display = Noritake()

        if options.line1:
            display.put_string((0, 0), options.line1, 'huge')

        if options.line2:
            display.put_string((46, 4), options.line2, 'medium')

        if options.out:
            display.image.save(options.out)
        else:
            display.show()
            
