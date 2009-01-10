#!/usr/bin/perl -w
#
use strict;
my ($filename,$dist,$pos,%wides,$cnt);

open IF,"<bacula.log" or die "Cannot open log file";

$cnt = 0;
while (<IF>) {
	chomp;
	$cnt++;

	# A new filename starts with a paren at the left margin,
	#  followed by a ./ .
	if (/^\(\.\/([\S]*tex)/) {
		$filename = $1; 
		next;
	}
	
	if (/^Overfull \\hbox \(([\d.]*)pt too wide.*lines (.*)$/) {
		($dist,$pos) = ($1,$2);
		$wides{$dist} = {filename => $filename,
						 pos => $pos,
						 logline => $cnt,
						 };
	}
}
close IF;


# print em
open OF,">overfulls" or die "Cannot open output file\n";
foreach (reverse(sort{$a<=>$b}(keys(%wides)))) {
	printf OF "%.5fpt line %s in file %s   line %d in logfile\n",$_,$wides{$_}{pos},$wides{$_}{filename},$wides{$_}{logline};
}
close OF;
print "Done\n";
