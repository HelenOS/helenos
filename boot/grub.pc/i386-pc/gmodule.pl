###
### Generate GDB commands, that load symbols for specified module,
### with proper section relocations. See .gdbinit
###
### $Id: gmodule.pl,v 1.2 2006/05/14 11:38:42 lkundrak Exp lkundrak $
### Lubomir Kundrak <lkudrak@skosi.org>
###

use strict;

while (<>) {
	my ($name, %sections) = split;

	print "add-symbol-file $name.module";

	open (READELF, "readelf -S $name.mod |") or die;
	while (<READELF>) {
		/\[\s*(\d+)\]\s+(\.\S+)/ or next;

		if ($2 eq '.text') {
			print " $sections{$1}";
			next;
		}

		print " -s $2 $sections{$1}"
			if ($sections{$1} ne '0x0' and $sections{$1} ne '');
	};
	close (READELF);
	print "\n";
}
