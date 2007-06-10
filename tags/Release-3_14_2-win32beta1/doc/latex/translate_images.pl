#!/usr/bin/perl -w
#
use strict;

# Opens the file imagename_translations and reads the contents into a hash.
# Then opens the html file(s) indicated in the command-line arguments and
#  changes all image references according to the translations described in the 
#  above file.  Finally, it renames the image files.
#
# Original creation: 3-27-05  by Karl Cunningham.
#
my $TRANSFILE = "imagename_translations";
my $path;

# Loads the contents of $TRANSFILE file into the hash referenced in the first 
#  argument.
sub read_transfile {
	my $trans = shift;

	open IN,"<$path$TRANSFILE" or die "Cannot open image translation file $path$TRANSFILE for reading\n";
	while (<IN>) {
		chomp;
		my ($new,$old) = split(/\001/);
		# The filename extension of the new one must be made to be the old one.
		my ($ext) = $new =~ /.*(\..*)$/;
		$old =~ s/(.*)\..*$/$1$ext/;
		$trans->{$new} = $old;
	}
	close IN;
}
	
# Translates the image names in the file given as the first argument, according to 
#  the translations in the hash that is given as the second argument.
#  The file contents are read in entirely into a string, the string is processed, and
#  the file contents are then written. No particular care is taken to ensure that the
#  file is not lost if a system failure occurs at an inopportune time.  It is assumed
#  that the html files being processed here can be recreated on demand.
sub translate_html {
	my ($filename,$trans,$filelist) = @_;
	my ($contents,$out,$this,$img,$dest);
	my $cnt = 0;

	# If the filename is an external link ignore it.  And drop any file:// from
	#  the filename.
	$filename =~ /^(http|ftp|mailto)\:/ and return 0;
	$filename =~ s/^file\:\/\///;
	# Load the contents of the html file.
	open IF,"<$path$filename" or die "Cannot open $path$filename for reading\n";
	while (<IF>) {
		$contents .= $_;
	}
	close IF;

	# Now do the translation...
	#  First, search for an image filename.
	while ($contents =~ /\<\s*IMG[^\>]*SRC=\"/si) {
		$contents = $';
		$out .= $` . $&;
		
		# The next thing is an image name.  Get it and translate it.
		$contents =~ /^(.*?)\"/s;
		$contents = $';
		$this = $&;
		$img = $1;
		# If the image is in our list of ones to be translated, do it
		#  and feed the result to the output.
		$cnt += $this =~ s/$img/$trans->{$img}/ if (defined($trans->{$img}));
		$out .= $this;
	}
	$out .= $contents;

	# Now send the translated text to the html file, overwriting what's there.
	open OF,">$path$filename" or die "Cannot open $path$filename for writing\n";
	print OF $out;
	close OF;

	# Now look for any links to other files.
	while ($out =~ /\<\s*A[^\>]*HREF=\"(.*?)\"/si) {
		$out = $';
		$dest = $1;
		# Drop an # and anything after it.
		$dest =~ s/\#.*//;
		$filelist->{$dest} = '' if $dest;
	}
	return $cnt;
}
	
# REnames the image files spefified in the %translate hash.
sub rename_images {
	my $translate = shift;
	my ($response);

	foreach (keys(%$translate)) {
		$response = `mv -f $path$_ $path$translate->{$_} 2>&1`;
		$response and die $response;
	}
}

#################################################
############# MAIN #############################
################################################

# %filelist starts out with keys from the @ARGV list.  As files are processed,
#  any links to other files are added to the %filelist.  A hash of processed
#  files is kept so we don't do any twice.

my (%translate,$search_regex,%filelist,%completed,$thisfile);
my $cnt;

(@ARGV) or die "ERROR: Filename(s) to process must be given as arguments\n";

# Use the first argument to get the path to the file of translations.
my $tmp = $ARGV[0];
($path) = $tmp =~ /(.*\/)/;
$path = '' unless $path;

read_transfile(\%translate);

foreach (@ARGV) {
	# Strip the path from the filename, and use it later on.
	s/(.*\/)//;
	$path = $1;
	$path = '' unless $path;
	$filelist{$_} = '';

	while ($thisfile = (keys(%filelist))[0]) {
		$cnt += translate_html($thisfile,\%translate,\%filelist) if (!exists($completed{$thisfile}));
		delete($filelist{$thisfile});
		$completed{$thisfile} = '';
	}
	print "$cnt links changed\n";
}

rename_images(\%translate);
