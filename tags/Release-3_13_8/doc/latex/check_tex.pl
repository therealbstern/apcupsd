#!/usr/bin/perl -w
# Finds multiple hyphens not inside a verbatim environment (or \verb).  
#   Places these inside a \verb{} contruct so they will not be converted
#   to single hyphen by latex or latex2html.

use strict;

my %args;


# The following builds the test string to identify and change multiple
#   hyphens in the tex files.  Several constructs are identified but only
#   multiple hyphens are changed; the others are fed to the output
#   unchanged.
my $b = '\\\\begin\\*?\\s*\\{\\s*';  # \begin{
my $e = '\\\\end\\*?\\s*\\{\\s*';    # \end{
my $c = '\\s*\\}';                   # closing curly brace

# # This captures entire verbatim environments.  These are passed to the output
# #   file unchanged.
my $verbatimenv = $b . "verbatim" . $c . ".*?" . $e . "verbatim" . $c;

# # This captures \verb{..{ constructs.  They are passed to the output unchanged.
my $verb = '\\\\verb\\*?(.).*?\\1';

# # This captures multiple hyphens with a leading and trailing space.  These are not changed.
my $hyphsp = '\\s\\-{2,}\\s';

# # This identifies other multiple hyphens.
my $hyphens = '\\-{2,}';

# This protects "protected" hyphen strings, such as for mdash and ndash.
my $protected_hyphens = '\\{\\-{2,}\\}';

# # This identifies \hyperpage{..} commands, which should be ignored.
my $hyperpage = '\\\\hyperpage\\*?\\{.*?\\}';

# # This builds the actual test string from the above strings.
# #my $teststr = "$verbatimenv|$verb|$tocentry|$hyphens";
my $teststr = "$verbatimenv|$verb|$hyphsp|$protected_hyphens|$hyperpage|$hyphens";


sub get_includes {
	# Get a list of include files from the top-level tex file.
	my (@list,$file);
	
	foreach my $filename (@_) {
		$filename or next;
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
	my ($linecnt,$filedata,$out,$this,$thiscnt,$before,$verbenv,$cnt);
	
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
		$linecnt = 1;

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

			$linecnt += $before =~ tr/\n/\n/;
			$linecnt += $this =~ tr/\n/\n/;
			if (exists $args{'change'}) {
				# Use this contruct for putting something in between each hyphen
				#$thiscnt += ($this =~ s/^\-+/do {join('\\,',split('',$&));}/e);

				# This is where the actual conversion is done.
				# Use this construct for putting something around each hyphen.
				$thiscnt += ($this =~ s/^\-+/\\verb\{$&\{/);
			} else {
				if ($this =~ /^\-+/) {
					$thiscnt++;
					print "Multiple hyphen found at line $linecnt in $file\n";
				}
			}
			
			# Put what came before and our (possibly) changed string into 
			#   the output buffer.
			$out .= $before . $this;
		}

		# If any hyphens were converted, save the file.
		if ($thiscnt and exists $args{'change'}) {
			open OF,">$file" or die "Cannot open output file $file";
			print OF $out . $filedata;
			close OF;
		}
		$cnt += $thiscnt;
	}
	return $cnt;
}

sub check_arguments {
	# Checks command-line arguments for ones starting with --  puts them into
	#   a hash called %args and removes them from @ARGV.
	my $args = shift;
	my $i;

	for ($i = 0; $i < $#ARGV; $i++) {
		$ARGV[$i] =~ /^\-+/ or next;
		$ARGV[$i] =~ s/^\-+//;
		$args{$ARGV[$i]} = "";
		delete ($ARGV[$i]);
		
	}
}

##################################################################
#                       MAIN                                  ####
##################################################################

my @includes;
my $cnt;

check_arguments(\%args);

# Examine the file pointed to by the first argument to get a list of 
#  includes to test.
@includes = get_includes(@ARGV);

$cnt = convert_hyphens(@includes);

if (exists $args{'change'}) {
	print "$cnt Multiple hyphen", ($cnt == 1) ? "" : "s"," Changed\n";
} else {
	print "$cnt Multiple hyphen", ($cnt == 1) ? "" : "s"," Found\n";
}
