#!/usr/bin/perl -w
#
#Modifies manual.tex to fix up several things:
#
#1. Changes section, subsection, etc. according to %mapping
#2. Changes the same things inside \addcontentsline commands.
#3. Add three spaces to the two lines following \item commands.

use strict;

my $INPUTFILE = "manual.tex";
my $OUTPUTFILE = "manual.tex";

my %mapping = ( 'section' => 'section',
			'subsection' => 'section',
			'subsubsection' => 'subsection',
			'paragraph' => 'subsubsection',
			'subparagraph' => 'paragraph',
			);
#\addcontentsline{toc}{subsubsection}

my $sectionsearch = "\\\\(";
my $tocsearch = "\\\\addcontentsline\\{toc\\}\\{(";
my $insidesearch = "(";
foreach (keys(%mapping)) {
	$sectionsearch .= "$_|";
	$tocsearch .= "$_|";
	$insidesearch .= "$_|";
}
$sectionsearch =~ s/\|$/)\\*?\\{/;
$tocsearch =~ s/\|$/)\\}/;
$insidesearch =~ s/\|$/)/;

# Read in the contents of the file.
my $text = "";
open IF, "<$INPUTFILE" or die "cannot open $INPUTFILE for reading";
while (<IF>) {
	$text .= $_;
}
print length($text)," bytes loaded\n";
close IF;

# Now process the text.
my $output = "";
my $this;
my $sectcnt = 0;
while ($text =~ /$sectionsearch/s) {
	$output .= $`;
	$text = $';
	$this = $&;
	$sectcnt += $this =~ s/$insidesearch/$mapping{$1}/;
	$output .= $this;
}
$text = $output . $text;

$output = "";
my $toccnt = 0;
while ($text =~ /$tocsearch/s) {
	$output .= $`;
	$text = $';
	$this = $&;
	$toccnt += $this =~ s/$insidesearch/$mapping{$1}/;
	$output .= $this;
}

$text = $output . $text;

# Next we look for a line that starts with \item, and indent the two next lines (if not blank)
#  by three spaces.
my $linecnt = 3;
my $itemcnt = 0;
$output = "";
foreach (split(/\n/,$text)) {
	$_ .= "\n"; # Put back the return.
	if ($linecnt < 3 and /^[^\n]/ and !/^\\item/) {
		$output .= "   " . $_;
		$itemcnt++;
		$linecnt++;
	} else {
		$linecnt = 3;
		$output .= $_;
	}
	/^\\item / and $linecnt = 1;
}
		
open OF,">$OUTPUTFILE" or die "Cannot open $OUTPUTFILE for writing";
print OF $output;
close OF;
print length($output)," bytes written\n";
print "$sectcnt sections changed.  $toccnt toc entries changed.\n";
print "Indented $itemcnt lines after \\item.\n";

