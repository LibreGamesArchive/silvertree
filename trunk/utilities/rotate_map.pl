use strict;
my $width = 0;
my @lines = ();
while(my $line = <>) {
	chomp $line;
	my @line = split /,/, $line;
	$width = scalar(@line) if scalar(@line) > $width;
	push @lines, \@line;
}

for(my $x = 0; $x < $width; ++$x) {
	for(my $y = 0; $y < scalar(@lines); ++$y) {
		print ", " if $y;
		my $line = $lines[$y];
		print $line->[$x];
	}
	print "\n";
}
