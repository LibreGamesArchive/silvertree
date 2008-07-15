while(<>) {
	s/\/\/.*//;
	if(/VertexBuffer/) {
		print;
		$_ = <>;
		print;
		while(<>) {
			last if /\}/;
			($x,$y,$z,$a,$b,$c,$u,$v) = split ' ', $_;
			my $scale = 1.0;
			$x *= $scale;
			$y *= $scale;
			$z *= -$scale;
			print "      $x $z $y $a $c $b $u $v\n";
		}
	}

	print;
}
