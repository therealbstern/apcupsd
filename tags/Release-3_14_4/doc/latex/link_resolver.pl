#!/usr/bin/perl -w
use strict;
#
# Straightens out links in multiple latex source files. Uses a master tex
#  file (first argument) and straightens out all the links. The problem is that
#  links in each tex file may point to other tex files.  But 
#  we aren't going to generate multiple dvi files in the end, so the links all
#  need to work in the same source file. This will be resolved by making a list
#  of links, a hash with keys being the original link targets. 
#  Then global link names will be created. The hash will contain the
#  new link name as the value. A reverse hash will also be created at the
#  same time.

# In addition to the above situation, it is possible that tex filenames have changed
#  since the links were created. To handle this, parameters can be entered
#  from the command line and links will be translated. Multiple translation arguments
#  can be entered at the command line to translated any number of file links.

# The top-level tex file is read in and all includes saved. These files are then
#  examined to find all link anchors. The list of links is processed to make them 
#  all unique across all source files. This is done by appending an integer to each
#  link name, using the lowest integer that will generate a unique link name.
#
# The tex files are then processed again, dropping the filename in the link, and
#  putting in the unique link name determined above. The new files are named
#  foo.linked.tex, where foo is the name of the original tex file.

# Unresolved links are listed to STDOUT, and forward and reverse lists of links
#  are output to files at the end.

# Invocation syntax is as follows:
#
#  link_resolver.pl -f infile.tex [ -t oldfile=newfile]...
#
#

my %translation;

sub max { return $_[0] > $_[1] ? $_[0] : $_[1]; }

sub get_includes {
	# Get a list of include files from the top-level tex file.
	my $masterFile = shift;
	my (@list,$file,%excludeList,$filelist);

	# Check that the master file exists, and open it.
	# Grab a list of include files, and add it to the array.
	open MASTER,"<$masterFile" or die "Cannot open master file: $masterFile";
	while (<MASTER>) {
		chomp;
		# If the special code '% nolinks' is found, parse the rest of the line for
		#   a list of files to not bring in to resolve links.
		if (($filelist) = /^\%\s+nolinks\s+(.*)/i) {
			foreach (split(/\s+/,$filelist)) {$excludeList{$_} = "";}
		}
			
		($file) = /^\\include\{(.*)?\}/ or next;
		# Remove the .linked part of the filename, that refers to files already linked.
		$file =~ s/\.linked$//;
		if (!defined $excludeList{$file}) {
			push(@list,"$file.tex");
		}
	}
	close MASTER;
	return @list;
}

sub get_anchors {
	# Examines each file and grabs a list of anchors from it. 
	my @files = @_;
	my $anchors = {};
	my ($cnt,$fileAnchors);

	# Creates a hash composed of those global links, one key for each file. Each
	#  of these hashes will contain a hash of anchors from the file.
	foreach my $file (@files) { 
		($fileAnchors,$cnt) = load_anchors($file); 
		$anchors->{$file} = $fileAnchors;
	}
	return $anchors;
}

sub load_anchors {
	# Loads the links from the given file into the anchors hash, and returs that hash.
	my ($filename) = shift;
	my ($bfr,$name,$cnt,$targetFile,$position,$data);
	my $anchors = {};

	# Opens the indicated file for reading.
	# Load the whole file into a buffer.
	# Search the buffer for the form \label{foo} or \special{html:<a name="foo">}.
	# Note that anchors may be duplicated in the two types of references, but that 
	#  doesn't matter.
	#   If found,
	#   	extract the name.
	#       Drop trailing spaces.
	#		save in anchors hash for this file.
	# close the file.
	# Return the anchors hash for this file.
	
	open IF,"<$filename" or die "Cannot open file $filename for reading";
	while (<IF>) {
		$bfr .= $_;
	}
	close IF;

	$cnt = 0;
	(!$bfr) and die "No data in Input File: $filename";
	while ($bfr =~ /(\\label\{.*?\})|(\\special\n*\{html:<a\s+name=\".*?\">\})/s ) {
		if ($bfr =~ /\\label\{(.*?)\}/s) {
			$bfr = $';
			$name = $1;
			$name =~ s/\s+$//;
			$anchors->{$name} = $name;
			$cnt++;
		} elsif ($bfr =~ /\\special\n*\{html:<a\s+name=\"(.*?)\">\}/s ) {
			$bfr = $';
			$name = $1;
			$name =~ s/\s+$//;
			$anchors->{$name} = $name;
			$cnt++;
		} else {
			die "Parse error retrieving anchors";
		}
	}
	return ($anchors,$cnt);
}


# convert_links
#  Go through the list of anchors for each file and convert the anchors to unique ones.
#  Save the new anchor in its original place and also into a hash of new anchors.
sub convert_links {
	# Converts the anchor to global (source-independent) versions.
	my ($links) = shift;
	my ($file,$anchor,$newanchor);
	my %linksList;

	foreach $file (keys(%$links)) {
		foreach $anchor (keys(%{$links->{$file}})) {
			$newanchor = get_newlink($anchor,\%linksList);
			$links->{$file}{$anchor} = $newanchor;
			$linksList{$newanchor} = "";
		}
	}
}

sub get_newlink {
	# Makes up a new anchor from the old one.
	my ($anchor,$linkhash) = @_;
	my ($newlink,$number,$base);

	# If the anchor isn't already in the hash, it's unique so return it.
	# Otherwise it's not unique, and we need to append a number.
	#  If the link already contains a number at the end, 
	#    start with one higher, otherwise start with 0.
	#  If the anchor exists with the number appended, keep incrementing
	#    the number until a unique one is found.
	# Return the new anchor.
	if (!defined($linkhash->{$anchor})) { return $anchor; };
	if ($anchor =~ /\d*$/) { 
		$number = $&;
		$base = $`;
	} else { 
		$number = 0; 
		$base = $anchor;
	}
	
	while (exists($linkhash->{$anchor . ++$number})) {}
	return $anchor . $number;
}

# changeOldLinks
#  For each source file in the list of files to process,
#    Process the file, writing links into an output file foo.linked.tex.
sub changeOldLinks {
	my ($links,$filelist) = @_;
	my ($infile,$response,$outfile);
	my $cnt = 0;

	foreach $infile (@$filelist) {
		$outfile = $infile;
		$outfile =~ s/\.tex/.linked.tex/;
		$cnt += changeFileLinks($infile,$outfile,$links);
	}
	return $cnt;
}

# Open the tex file for reading, and the output file for writing.
# Read the entire target file into a buffer.
# Pass everything from the target file to the output file, until we
#   get to a link or a label
# If the reference is an external one (http:, ftp:, or mailto:) ignore
#   it and pass it unchanged to the output file
# If it is a label, save the command and grab the first argument as the target.
# If it is a hyperref, save the command and srab the fourth argument as the target
# Separate the filename from the anchor in the target.
# Translate any filenames that have changed.
# If the reference has no corresponding anchor, output a warning.
# If the reference is to a local file that is not a tex file, output a warning.
# Output the reference to the output file, using the new reference name. No line wrapping
#  occurs for outputting a referenece, so the lines in the tex file may get longer.

sub changeFileLinks {
	my ($infile,$outfile,$links) = @_;
	my ($bfr,$cmd,$type,$target,$refFile,$pointer,$newtarget);
	my ($url,$category,$name,$temp);
	my ($text,$output);
	my $cnt = 0;

	my @regLink;

	$regLink[0] = '(\\\\elink\\{)(.*?)\\}';
	$regLink[1] = '(\\\\label\\{)(.*?)\\}';
	$regLink[2] = '(\\\\hyperref\\{)(.*?)\\}\\{(.*?)\\}\\{(.*?)\\}\\{(.*?)\\}';
	$regLink[3] = '(\\\\special\\n*\\{html:<a\\s+href=\\")(.*?)\\">\\}';
	$regLink[4] = '(\\\\special\\n*\\{html:<a\\s+name=\\")(.*?)\\">\\}';
	$regLink[5] = '(\\\\ilink\\{)(.*?)\\}\\{(.*?)\\}';
	my $linkTest = "(" . join(")|(",@regLink) . ")";

	open TEX,"<$infile" or die "Cannot open $infile for reading\n";
	open OUT,">$outfile" or die "Cannot open $outfile for writing\n";
	while (<TEX>) {
		$bfr .= $_;
	}
	close TEX;

	while ($bfr =~ /$linkTest/so) {
		$bfr = $& . $';
		$cnt++;
		print OUT $`;
		if (($cmd,$target) = ($bfr =~ /^$regLink[0]/so)) {
			$bfr = $';
			print OUT $&;
			next;
		} elsif (($cmd,$target) = $bfr =~ /^$regLink[1]/so) {
			$bfr = $';
			$refFile = $infile; $pointer = $target; 
			$type = 1;
		} elsif (($cmd,$text,$category,$name,$url) =  $bfr =~ /^$regLink[2]/so ) {
			$bfr = $';
			$refFile = $url;
			$pointer = $category;
			$type = 2;
		} elsif (($cmd,$target) =  $bfr =~ /^$regLink[3]/so) {
			$bfr = $';
			$temp = $&;
			if ($target =~ /^(http:|ftp:|mailto:)/) {
				print OUT $temp;
				next;
			}
			($refFile,$pointer) = split (/\#+/,$target);
			$type = 3;
		} elsif (($cmd,$target) =  $bfr =~ /^$regLink[4]/so) {
			$bfr = $';
			$refFile = $infile; $pointer = $target; 
			$type = 4;
		} elsif (($cmd,$text,$target) =  $bfr =~ /^$regLink[5]/so ) {
			$bfr = $';
			($refFile,$pointer) = split (/\#+/,$target);
			$type = 5;
		} else {
			# Error.
			die "Parsing Error";	
		}
		$cmd =~ s/\n/ /sg;

		foreach (keys(%translation)) { $refFile =~ s/$_/$translation{$_}/; }

		if (defined($pointer) and $pointer) {
			$pointer =~ s/\n//g;
			$pointer =~ s/\s+$//;
			if (defined($links->{$refFile}{$pointer})) {
				$newtarget = $links->{$refFile}{$pointer};
			} else {
				warn "Warning: Reference not Found- $refFile#$pointer in  $infile\n";
				$newtarget = "";
			}
		} else {
			warn "Warning: Outside Reference: $refFile in  $infile\n";
			$newtarget = $refFile;
		}


		if ($type == 0) {
			$cmd = "";
		} elsif ($type == 1) {
			$cmd .= "$newtarget\}";
		} elsif ($type == 2) {
			$cmd .= "$text\}\{\}\{\}\{$newtarget\}";
		} elsif ($type == 3) {
			$cmd .= "$newtarget\">\}";
		} elsif ($type == 4) {
			$cmd .= "$newtarget\">\}";
		} elsif ($type == 5) {
			$cmd .= "$text\}\{$newtarget\}";
		} else {
			die "Parsing Error";
		}
		print OUT $cmd;
	}
	print OUT $bfr;
	close OUT;
	return $cnt;
}


# Write a list of links to the indicated file.
sub write_links_file {
	my ($links) = shift;
	my $outlinks = "links.out";
	my ($strlen,$filename,$pointer,$longest,$spaces);

	# Writes the list of links out to a file.
	open OF,">$outlinks" or die "Cannot open $outlinks for writing\n";

	# Find the longest key.
	$longest = 0;
	foreach $filename (keys(%$links)) {
		foreach $pointer (sort(keys(%{$links->{$filename}}))) {
			$strlen = length($filename) + length($pointer);
			$longest = max($longest,$strlen);
		}
	}

	foreach $filename (sort(keys(%$links))) {
		foreach $pointer (sort(keys(%{$links->{$filename}}))) {
			$strlen = length($filename) + length($pointer);
			$spaces = " " x ($longest - $strlen);
			print OF "$filename#$pointer $spaces $links->{$filename}{$pointer}\n";
		}
	}
	close OF;
}

# Write a list of reverse links to the indicated file.
sub writeRevLinks {
	my ($links) = shift;
	my $outlinks = "linksr.out";
	my (%revlinks,$longest,$spaces);

	# Writes the list of reverse links out to a file.
	open OF,">$outlinks" or die "Cannot open $outlinks for writing\n";

	foreach my $filename (sort(keys(%$links))) {
		foreach my $pointer (sort(keys(%{$links->{$filename}}))) {
			$revlinks{$links->{$filename}{$pointer}} = "$filename#$pointer";
		}
	}

	$longest = 0;
	foreach (keys(%revlinks)) {
		$longest = max($longest,length($_));
	}

	foreach (sort(keys(%revlinks))) {
		$spaces = " " x ($longest - length($_));
		print OF "$_ $spaces $revlinks{$_}\n";
	}
	close OF;
}

# Look for arguments in the command line, and decode them into a hash.
sub parse_cmdline {
	my $cmds = {};
	my ($cnt,$arg);

	$cnt = 0;
	while (defined($arg = $ARGV[$cnt++])) {
		if ($arg =~ /-f/) {
			if (defined($arg = $ARGV[$cnt++])) { $cmds->{infile} = $arg; }
				else {die "No Input File given on Command line\n"; }
		} elsif ($arg =~ /-t/) {
			if (defined($arg = $ARGV[$cnt++])) { 
				if ($arg =~ /=/) { $translation{$`} = $'; }
					else {die "Invalid translation Given $arg"; }
			} else {
				die "No Translation argument given for -t argument";
			}
		}
	}
	return $cmds;
}


##################################################################
#                       MAIN                                  ####
##################################################################

my (@includes,%pointers,%pointersRev,$anchors);

# Parse the command-line arguments and put them into a hash.
my $args = parse_cmdline;

if (!defined($args->{infile})) {
	die "Master File to Process must be given with -f parameter\n";
}

foreach (sort(keys(%translation))) {
	print "Filename Translation: $_ -> $translation{$_}\n";
}

# Read in the list of files to be included
# Get the links from each file.
# Convert the link format from local to global; check for duplicates.
# Change all the links in the source files to global ones, and change
# 	the names (labels) to global ones.
# Write out the list of links and reverse links to a text file.
#
@includes = get_includes($args->{infile});

$anchors = get_anchors(@includes);
convert_links($anchors);
my $link_cnt = changeOldLinks($anchors,\@includes);
#write_links_file($anchors);
#writeRevLinks($anchors);
my $anchor_cnt = keys(%$anchors);
print "Finished -- $anchor_cnt Anchors Found  $link_cnt Links Resolved\n";
