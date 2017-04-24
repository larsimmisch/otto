#!/usr/bin/env python

import sys
from optparse import OptionParser
from PIL import Image

def convert_byte(values):
    value = 0
    bit = 128
    if not len(values) == 8:
        raise ValueError, "need exacly 8 values"

    for v in values:
        if v == 0:
            value = value | bit
        bit = bit >> 1

    return value

if __name__ == '__main__':

    parser = OptionParser(usage='usage: %prog [options] [<filename>]')
    parser.add_option('-o', '--out', type='string')

    options, args = parser.parse_args()

    if len(args) > 1:
        parser.print_help()
        sys.exit(2)

    if options.out:
        outfile = open(options.out, 'w')
    else:
        outfile = sys.stdout

    im = Image.open(args[0])
    if im.size != (112, 16):
        print "Image must be 112*16"
        sys.exit(1)

    if im.mode != 'RGB':
        print "Image must be 112*16"
        sys.exit(1)

    im = im.convert('1')
    pixels = im.load()

    for l in (0, 1):
        outfile.write('\nconst char line%d[] PROGMEM = {' % l)
        for x in range(im.size[0]):
            if x % 8 == 0:
                outfile.write('\n    ')

            values = [pixels[x, y + 8 * l] for y in range(8)]
            outfile.write('0x%02x, ' % convert_byte(values))

        outfile.write('\n};\n')
