#!/usr/bin/perl -w
# Finds multiple hyphens not inside a verbatim environment (or \verb).  
#   Places these inside a \verb{} contruct so they will not be converted
#   to single hyphen by latex or latex2html.

use strict;

# The following builds the test string to identify and change multiple
#   hyphens in the tex files.  Several constructs are identified but only
#   multiple hyphens are changed; the others are fed to the output 
#   unchanged.
my $b = '\\\\begin\\*?\\s*\\{\\s*';  # \begin{
my $e = '\\\\end\\*?\\s*\\{\\s*';    # \end{
my $c = '\\s*\\}';                   # closing curly brace

# This captures entire verbatim environments.  These are passed to the output
#   file unchanged.
my $verbatimenv = $b . "verbatim" . $c . ".*?" . $e . "verbatim" . $c;  

# This captures \verb{} constructs.  They are passed to the output unchanged.
my $verb = '\\\\verb\\*?(.).*?\\1';

# This identifies multiple hyphens.
my $hyphens = '\\-{2,}';

# This builds the actual test string from the above strings.
#my $teststr = "$verbatimenv|$verb|$tocentry|$hyphens";
my $teststr = "$verbatimenv|$verb|$hyphens";


sub get_includes {
	# Get a list of include files from the top-level tex file.
	my (@list,$file);
	
	while (my $filename = shift) {
		# Start with the top-level latex file so it gets checked too.
		push (@list,$filename);

		# Get a list of all the html files in the directory.
		open IF,"<$filename" or die "Cannot open input file $filename";
		while (<IF>) {
			chomp;
			push @list,"$1.tex" if (/\\include\{(.*?)\}/);
		}

		close IF;
	}
	return @list;
}

sub convert_hyphens {
	my (@files) = @_;
	my ($filedata,$out,$this,$thiscnt,$before,$verbenv,$cnt);
	
	# Build the test string to check for the various environments.
	#   We only do the conversion if the multiple hyphens are outside of a 
	#   verbatim environment (either \begin{verbatim}...\end{verbatim} or 
	#   \verb{--}).  Capture those environments and pass them to the output
	#   unchanged.

	$cnt = 0;
	foreach my $file (@files) {
		# Open the file and load the whole thing into $filedata. A bit wasteful but
		#   easier to deal with, and we don't have a problem with speed here.
		$filedata = "";
		open IF,"<$file" or die "Cannot open input file $file";
		while (<IF>) {
			$filedata .= $_;
		}
		close IF;
		
		# Set up to process the file data.
		$out = "";
		$verbenv = 0;
		$thiscnt = 0;

		# Go through the file data from beginning to end.  For each match, save what
		#   came before it and what matched.  $filedata now becomes only what came 
		#   after the match.
		#   Chech the match to see if it starts with a multiple-hyphen.  If so
		#     change it to \verb{--}. The other possible matches in the pattern
		#     won't start with a hyphen, so we're ok with matching that.
		while ($filedata =~ /$teststr/os) {
			$this = $&;
			$before = $`;
			$filedata = $';
			# This is where the actual conversion is done.

			# Use this contruct for putting something in between each hyphen
			#$thiscnt += ($this =~ s/^\-+/do {join('\\,',split('',$&));}/e);

			# Use this construct for putting something around each hyphen.
			$thiscnt += ($this =~ s/^\-+/\\verb\{$&\{/);
			
			# Put what came before and our (possibly) changed string into 
			#   the output buffer.
			$out .= $before . $this;
		}

		# If any hyphens were converted, save the file.
		if ($thiscnt) {
			open OF,">$file" or die "Cannot open output file $file";
			print OF $out . $filedata;
			close OF;
		}
		$cnt += $thiscnt;
	}
	return $cnt;
}
##################################################################
#                       MAIN                                  ####
##################################################################

my (@includes);
my $cnt;

# Examine the file pointed to by the first argument to get a list of 
#  includes to test.
@includes = get_includes(@ARGV);

$cnt = convert_hyphens(@includes);

print "$cnt Multiple hyphen", ($cnt == 1) ? "" : "s"," Found\n";
