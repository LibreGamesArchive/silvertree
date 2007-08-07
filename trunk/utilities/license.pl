while(my $arg = shift @ARGV) {
	open FILE, "<$arg" or die "could not open '$arg': $!";
	my @lines = <FILE>;
	close FILE;

	open OUT, ">$arg" or die "Could not write to '$arg': $!";
	print OUT "
/*
   Copyright (C) 2007 by David White <dave@whitevine.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License version 2 or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/
";
	print OUT @lines;
	close OUT;
}
