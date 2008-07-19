my $width = 200;
my $height = 200;
my $nhills = 0;
my $max_radius = 10;
my $forest = 0;

while(my $arg = shift @ARGV) {
	if($arg eq '--dim') {
		my $dim = shift @ARGV;
		($width,$height) = split /x/, $dim;
	} elsif($arg eq '--hills') {
		$nhills = shift @ARGV;
	} elsif($arg eq '--hillsize') {
		$max_radius = shift @ARGV;
	} elsif($arg eq '--forest') {
		$forest = shift @ARGV;
	} else {
		print STDERR "Options:
--dim WxH: specifies map dimensions
--hills n: specifies number of hills
--hillsize n: specifies size of hills
--forest n: specifies the % chance each tile will have a tree on it.
";
		exit;
	}
}

my @heightmap = ();
for(my $i = 0; $i != $width; ++$i) {
	push @heightmap, [(0) x $height];
}

for(my $n = 0; $n != $nhills; ++$n) {
	print STDERR "hill $n/$nhills\n";
	my $x = rand($width);
	my $y = rand($height);
	my $radius = rand($max_radius);
	&apply_hill($x,$y,$radius);
}

print '[scenario]
map_data="';

foreach my $row (@heightmap) {
	my @output = ();
	foreach my $tile (@$row) {
		my $itile = int($tile);
		my $f = '';
		$f = ' F' if rand(100) < $forest;
		push @output, "$itile h$f";
	}

	print join ",", @output;
	print "\n";
}
print '"
[/scenario]
';

sub apply_hill
{
	my ($xloc,$yloc,$radius) = @_;
	for(my $x = 0; $x != $width; ++$x) {
		for(my $y = 0; $y != $height; ++$y) {
			my $xdiff = ($xloc - $x);
			my $ydiff = ($yloc - $y);

			my $height = $radius - sqrt($xdiff*$xdiff + $ydiff*$ydiff);
			if($height > 0) {
				$heightmap[$x][$y] += $height;
			}
		}
	}
}
