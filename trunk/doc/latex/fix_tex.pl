#!/usr/bin/perl -w
# Finds multiple hyphens not inside a verbatim environment (or \verb).  
#   Places these inside a \verb{} contruct so they will not be converted
#   to single hyphen by latex or latex2html.

use strict;

my %args;


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

sub convert_files {
	my (@files) = @_;
	my ($linecnt,$filedata,$output,$thiscnt,$cnt);
	
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
		$thiscnt = 0;

		# We look for a line that starts with \item, and indent the two next lines (if not blank)
		#  by three spaces.
		my $linecnt = 3;
		my $itemcnt = 0;
		$output = "";
		foreach (split(/\n/,$filedata)) {
			$_ .= "\n"; # Put back the return.
			# If this line is less than the third line past the \item command,
			#  and the line isn't blank and doesn't start with whitespace
			#  add three spaces to the start of the line. Keep track of the number
			#  of lines changed.
			if ($linecnt < 3 and !/^\\item/) {
				if (/^[^\n\s]/) {
					$output .= "   " . $_;
					$thiscnt++;
				} else {
					$output .= $_;
				}
				$linecnt++;
			} else {
				$linecnt = 3;
				$output .= $_;
			}
			/^\\item / and $linecnt = 1;
		}
	
		# If any hyphens were converted, save the file.
		if ($thiscnt) {
			open OF,">$file" or die "Cannot open output file $file";
			print OF $output;
			close OF;
			print "$thiscnt line", ($thiscnt == 1) ? "" : "s"," Changed in $file\n";
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

$cnt = convert_files(@includes);
$cnt or print "No lines changed\n";
