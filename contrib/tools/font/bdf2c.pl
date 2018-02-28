#!/usr/bin/perl -w

#
# Copyright (c) 2000 Dmitry Bolkhovityanov
# Copyright (c) 2009 Martin Decky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# - Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# - Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# - The name of the author may not be used to endorse or promote products
#   derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

use strict;

my $skip;

my $width;
my $height;
my $offset_x;
my $offset_y;

my $gwidth;
my $gheight;
my $goffset_x;
my $goffset_y;

my $index;
my @glyphs;
my @chars;

open(BDF, "u_vga16.bdf") or die("Unable to open source font\n");

READHEADER: while (<BDF>) {
	/^FONTBOUNDINGBOX\s/ and do {
		($skip, $width, $height, $offset_x, $offset_y) = (split);

		die("Font width is not 8px\n") if ($width != 8);
		die("Font height is not 16px\n") if ($height != 16);
	};
	/^CHARS\s/ && last READHEADER;
}

READCHARS: while (<BDF>) {
	/^ENCODING\s+([0-9]+)\s*$/ && do {
		$index = $1;
	};
	/^BBX\s/ && do {
		($skip, $gwidth, $gheight, $goffset_x, $goffset_y) = (split);
	};
	/^BITMAP/ && do {
		my @glyph = ();
		my $y;

		# Add empty lines at top
		my $empties = $height + $offset_y - $goffset_y - $gheight;

		for ($y = 0; $y < $empties; $y++) {
			$glyph[$y] = 0;
		}

		# Scan the hex bitmap
		for ($y = $empties; $y < $empties + $gheight; $y++) {
			$_ = <BDF>;
			$glyph[$y] = hex(substr($_, 0, 2)) >> $goffset_x;
		}

		# Add empty lines at bottom
		my $fill = $height - $gheight - $empties;
		for ($y = $empties + $gheight; $y < $empties + $gheight + $fill; $y++) {
			$glyph[$y] = 0;
		}

		if ($index != 0) {
			$glyphs[$index] = (\@glyph);
			push(@chars, $index);
		}
	};
	/^ENDFONT/ && last READCHARS;
}

close(BDF);

@chars = sort { $a <=> $b } (@chars);

print "#define FONT_GLYPHS     " . (@chars + 1). "\n";
print "#define FONT_SCANLINES  " . $height . "\n";

print "\n";
print "uint16_t fb_font_glyph(const wchar_t ch)\n";
print "{\n";
print "\tif (ch == 0x0000)\n";
print "\t\treturn 0;\n\n";

my $pos = 0;
my $start = -1;
my $start_pos = 0;
my $prev = 0;
for $index (@chars) {
	if ($prev + 1 < $index) {
		if ($start != -1) {
			if ($start == $prev) {
				printf "\tif (ch == 0x%.4x)\n", $start;
				print "\t\treturn " . $start_pos . ";\n";
			} else {
				printf "\tif ((ch >= 0x%.4x) && (ch <= 0x%.4x))\n", $start, $prev;
				print "\t\treturn (ch - " . ($start - $start_pos) . ");\n";
			}

			print "\t\n";
		}

		$start = $index;
		$start_pos = $pos;
	}

	$pos++;
	$prev = $index;
}

print "\treturn " . @chars . ";\n";
print "}\n";

print "\n";
print "uint8_t fb_font[FONT_GLYPHS][FONT_SCANLINES] = {";

for $index (@chars) {
	print "\n\t{";

	my $y;
	for ($y = 0; $y < $height; $y++) {
		print ", " if ($y > 0);
		printf "0x%.2x", $glyphs[$index]->[$y];
	}

	print "},";
}

print "\n\t\n\t/* Special glyph for unknown character */\n\t{";
my $y;
for ($y = 0; $y < $height; $y++) {
	print ", " if ($y > 0);
	printf "0x%.2x", $glyphs[63]->[$y];
}

print "}\n};\n";
