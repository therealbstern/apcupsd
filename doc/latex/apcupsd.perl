# Provides support for latex2html for additional commands needed for 
#  the apcupsd manual.



# Returns the minimum of any number of numeric arguments.
sub min {
	my $tmp = shift;
	while ($test = shift) {
		$tmp = $test if ($test < $tmp);
	}
	return $tmp;
}

# These two are copied from 
#   /usr/lib/latex2html/style/hthtml.perl,
#   from the subroutine do_cmd_htmladdnormallink.
#   They have been renamed, then removed the 
#   name argument and reversed the other two arguments.
sub do_cmd_elink{
	local($_) = @_;
	local($text, $url, $href);
	local($opt, $dummy) = &get_next_optional_argument;
	$text = &missing_braces unless
		((s/$next_pair_pr_rx/$text = $2; ''/eo)
		||(s/$next_pair_rx/$text = $2; ''/eo));
	$url = &missing_braces unless
		((s/$next_pair_pr_rx/$url = $2; ''/eo)
		||(s/$next_pair_rx/$url = $2; ''/eo));
	$*=1; s/^\s+/\n/; $*=0;
	$href = &make_href($url,$text);
	print "\nHREF:$href" if ($VERBOSITY > 3);
	join ('',$href,$_);
}

sub do_cmd_ilink {
	local($_) = @_;
	local($text);
	local($opt, $dummy) = &get_next_optional_argument;
	$text = &missing_braces unless
			((s/$next_pair_pr_rx/$text = $2; ''/eo)
			||(s/$next_pair_rx/$text = $2; ''/eo));
	&process_ref($cross_ref_mark,$cross_ref_mark,$text);
}

sub do_cmd_lt { join('',"\&lt;",$_[0]); }
sub do_cmd_gt { join('',"\&gt;",$_[0]); }

# KEC  Copied from latex2html.pl and modified to prevent 
#  filename collisions. This is done with a static hash of 
#  already-used filenames. An integer is appended to the 
#  filename if a collision would result without it.
#  The addition of the integer is done by removing 
#  character(s) before .html if adding the integer would result
#  in a filename longer than 32 characters. Usually just removing
#  the character before .html would resolve the collision, but we
#  add the integer anyway. The first integer that resolves the 
#  collision is used.
# If a filename is desired that is 'index.html' or any case 
#  variation of that, it is changed to index_page.html, 
#  index_page1.html, etc.

#RRM  Extended to allow customised filenames, set $CUSTOM_TITLES
#     or long title from the section-name, set $LONG_TITLES
#
{ my %used_names; # Static hash.
sub make_name {
	local($sec_name, $packed_curr_sec_id) = @_;
	local($title,$making_name,$saved) = ('',1,'');
	my $final_name;
	if ($LONG_TITLES) {
		$saved = $_;
		&process_command($sections_rx, $_) if /^$sections_rx/;
		$title = &make_apcupsd_title($TITLE)
			unless ((! $TITLE) || ($TITLE eq $default_title));
		$_ = $saved;
	} elsif ($CUSTOM_TITLES) {
		$saved = $_;
		&process_command($sections_rx, $_) if /^$sections_rx/;
		$title = &custom_title_hook($TITLE)
			unless ((! $TITLE) || ($TITLE eq $default_title));
		$_ = $saved;
	}
	if ($title) {
		#ensure no more than 32 characters, including .html extension
		$title =~ s/^(.{1,27}).*$/$1/;
		++$OUT_NODE;
		$final_name = join("", ${PREFIX}, $title, $EXTN);
	} else {
		# Remove 0's from the end of $packed_curr_sec_id
		$packed_curr_sec_id =~ s/(_0)*$//;
		$packed_curr_sec_id =~ s/^\d+$//o; # Top level file
		$final_name = join("",($packed_curr_sec_id ?
			"${PREFIX}$NODE_NAME". ++$OUT_NODE : $sec_name), $EXTN);
	}

	# Change the name from index to index_page to avoid conflicts with
	#  index.html.
	$final_name =~ s/^(index)\.html$/$1_Page.html/i;

	# If the $final_name is already used, put an integer before the
	#     #  .html to make it unique.
	my $integer = 0;
	my $saved_name = $final_name;
	while (exists($used_names{$final_name})) {
		$final_name = $saved_name;
		my ($filename,$ext) = $final_name =~ /(.*)(\..*)$/;
		my $numlen = length(++$integer);

		# If the filename (after adding the integer) would be longer than
		#  32 characters, insert the integer within it.
		if (((my $namelen = length($final_name)) + $numlen) >= 32) {
			substr($filename,-$numlen) = $integer;
		} else {
			$filename .= $integer;
		}
		$final_name = $filename . $ext;
	}

	# Save the $final_name in the hash to mark it as being used.
	$used_names{$final_name} = undef;
	return $final_name;
}
}

sub make_apcupsd_title {
    local($_)= @_;
	local($num_words) = $LONG_TITLES;
	#RRM:  scan twice for short words, due to the $4 overlap
	#      Cannot use \b , else words break at accented letters
	$_ =~ s/(^|\s)\s*($GENERIC_WORDS)(\'|(\s))/$4/ig;
	$_ =~ s/(^|\s)\s*($GENERIC_WORDS)(\'|(\s))/$4/ig;
	#remove leading numbering, unless that's all there is.
	local($sec_num);
	if (!(/^\d+(\.\d*)*\s*$/)&&(s/^\s*(\d+(\.\d*)*)\s*/$sec_num=$1;''/e))
		{ $num_words-- };
	&remove_markers; s/<[^>]*>//g; #remove tags
	#revert entities, etc. to TeX-form...
	s/([\200-\377])/"\&#".ord($1).";"/eg;
	$_ = &revert_to_raw_tex($_);

	# get $LONG_TITLES number of words from what remains
	$_ = &get_apcupsd_words($_, $num_words) if ($num_words);
	# ...and cleanup accents, spaces and punctuation
	$_ = join('', ($SHOW_SECTION_NUMBERS ? $sec_num : ''), $_);
	s/\\\W\{?|\}//g;
	s/\s/_/g;
	s/\'s/s/ig; # Replace 's with just the s.
	s/\W/_/g;
	s/__+/_/g;
	s/_+$//;
	$_;
}

	#JCL(jcl-tcl)
	# changed completely
	# KEC 2-21-05  Changed completely again. 
	#
	# We take the first real words specified by $min from the string.
	#  REmove all markers and markups.
	# Split the line into words.
	# Determine how many words we should process.
	# Return if no words to process.
	# Determine lengths of the words.
	# Reduce the length of the longest words in the list until the
	#  total length of all the words is acceptable.
	# Put the words back together and return the result.
	#
sub get_apcupsd_words {
	local($_, $min) = @_;
	local($words,$i);
	local($id,%markup);
	# KEC
	my ($oalength,@lengths,$last,$thislen);
	my $maxlen = 28;

	#no limit if $min is negative
	$min = 1000 if ($min < 0);

	&remove_anchors;
	#strip unwanted HTML constructs
	s/<\/?(P|BR|H)[^>]*>/ /g;
	#remove leading white space and \001 characters
	s/^\s+|\001//g;
	#lift html markup
	s/(<[^>]*>(#[^#]*#)?)//ge;

	# Split $_ into a list of words.
	my @wrds = split /\s+|\-{3,}/;
	$last = &min($min - 1,$#wrds);
	return '' if ($last < 0);

	# Get a list of word lengths up to the last word we're to process.
	#  Add one to each for the separator.
	@lengths = map (length($_)+1,@wrds[0..$last]);

	$thislen = $maxlen + 1; # One more than the desired max length.
	do {
		$thislen--;
		@lengths = map (&min($_,$thislen),@lengths);
		$oalength = 0;
		foreach (@lengths) {$oalength += $_;}
	} until ($oalength <= $maxlen);
	$words = join(" ",map (substr($wrds[$_],0,$lengths[$_]-1),0..$last));
	return $words;
}

sub do_cmd_htmlfilename {
	my $input = shift;

	my ($id,$filename) = $input =~ /^<#(\d+)#>(.*?)<#\d+#>/;
}

# KEC 2-26-05
# do_cmd_addcontentsline adds support for the addcontentsline latex command. It evaluates
#  the arguments to the addcontentsline command and determines where to put the information.  Three
#  global lists are kept: for table of contents, list of tables, and list of figures entries.
#  Entries are saved in the lists in the order they are encountered so they can be retrieved
#  in the same order.
my (%toc_data);
sub do_cmd_addcontentsline { 
	&do_cmd_real_addcontentsline(@_); 
}
sub do_cmd_real_addcontentsline {
    my $data = shift;
	my ($extension,$pat,$unit,$entry);

    # The data is sent to us as fields delimited by their ID #'s.  Extract the
    #  fields.  The first is the extension of the file to which the cross-reference
	#  would be written by LaTeX, such as {toc}, {lot} or {lof}.  The second is either
	#  {section}, {subsection}, etc. for a toc entry, or , {table}, or {figure} 
	#  for a lot, or lof extension (must match the first argument), and 
	#  the third is the name of the entry.  The position in the document represents
	#  and anchor that must be built to provide the linkage from the entry.
    $extension = &missing_braces unless (
    ($data =~ s/$next_pair_pr_rx/$extension=$2;''/eo)
    ||($data =~ s/$next_pair_rx/$extension=$2;''/eo));
    $unit = &missing_braces unless (
    ($data =~ s/$next_pair_pr_rx/$unit=$2;''/eo)
    ||($data =~ s/$next_pair_rx/$unit=$2;''/eo));
    $entry = &missing_braces unless (
    ($data =~ s/$next_pair_pr_rx/$pat=$1;$entry=$2;''/eo)
    ||($data =~ s/$next_pair_rx/$pat=$1;$entry=$2;''/eo));

	$contents_entry = &make_contents_entry($extension,$pat,$entry,$unit);
	return ($contents_entry . $data);
}

# Creates and saves a contents entry (toc, lot, lof) to strings for later use, 
#  and returns the entry to be inserted into the stream.
# 
sub make_contents_entry {
    local($extension,$br_id, $str, $unit) = @_;
	my $words = '';
	my ($thisref);

    # If TITLE is not yet available use $before.
    $TITLE = $saved_title if (($saved_title)&&(!($TITLE)||($TITLE eq $default_title)));
    $TITLE = $before unless $TITLE;
    # Save the reference
    if ($SHOW_SECTION_NUMBERS) { 
		$words = &get_first_words($TITLE, 1);
	} else { 
		$words = &get_first_words($TITLE, 4);
	}
	$words = 'no title' unless $words;

	#
	# any \label in the $str will have already
	# created a label where the \addcontentsline occurred.
	# This has to be removed, so that the desired label 
	# will be found on the toc page.
	#
	if ($str =~ /tex2html_anchor_mark/ ) {
		$str =~ s/><tex2html_anchor_mark><\/A><A//g;
	}
	#
	# resolve and clean-up the hyperlink entries 
	# so they can be saved
	#
	if ($str =~ /$cross_ref_mark/ ) {
		my ($label,$id,$ref_label);
		$str =~ s/$cross_ref_mark#([^#]+)#([^>]+)>$cross_ref_mark/
			do { ($label,$id) = ($1,$2);
			$ref_label = $external_labels{$label} unless
			($ref_label = $ref_files{$label});
			'"' . "$ref_label#$label" . '">' .
			&get_ref_mark($label,$id)}
		/geo;
	}
	$str =~ s/<\#[^\#>]*\#>//go;
	#RRM
	# recognise \char combinations, for a \backslash
	#
	$str =~ s/\&\#;\'134/\\/g;		# restore \\s
	$str =~ s/\&\#;\`<BR> /\\/g;	#  ditto
	$str =~ s/\&\#;*SPMquot;92/\\/g;	#  ditto

	$thisref = &make_named_href('',"$CURRENT_FILE#$br_id",$str);
	$thisref =~ s/\n//g;

	# Now we build the actual entry that will go in the lot and lof.
	# If this is the first entry, we have to put a leading newline.
	if ($unit eq 'table' ) {
		if (!$table_captions) { $table_captions = "\n";}
		$table_captions .= "<LI>$thisref\n";
	} elsif ($unit eq 'figure') {
		if (!$figure_captions) { $figure_captions = "\n"; }
		$figure_captions .= "<LI>$thisref\n";
	}
    "<A NAME=\"$br_id\">$anchor_invisible_mark<\/A>";
}

# This is needed to keep latex2html from trying to make an image for the registered
#  trademark symbol (R).  This wraps the command in a deferred wrapper so it can be
#  processed as a normal command later on.  If this subroutine is not put in latex2html
#  invokes latex to create an image for the symbol, which looks bad.
sub wrap_cmd_textregistered {
    local($cmd, $_) = @_;
    (&make_deferred_wrapper(1).$cmd.&make_deferred_wrapper(0),$_)
}

# KEC
# Copied from latex2html.pl and modified to create a file of image translations.
#  The problem is that latex2html creates new image filenames like imgXXX.png, where
#  XXX is a number sequentially assigned.  This is fine but makes for very unfriendly 
#  image filenames. I looked into changing this behavior and it seems very much embedded
#  into the latex2html code, not easy to change without risking breaking something.
# So I'm taking the approach here to write out a file of image filename translations,
#  to reference the original filenames from the new filenames.  THis was post-processing
#  can be done outside of latex2html to rename the files and substitute the meaningful
#  image names in the html code generated by latex2html.  This post-processing is done
#  by a program external to latex2html.
#
# What we do is this: This subroutine is called to output images.tex, a tex file passed to 
#  latex to convert the original images to .ps.  The string $latex_body contains info for 
#  each image file, in the form of a unique id and the orininal filename.  We extract both, use
#  the id is used to look up the new filename in the %id_map hash.  The new and old filenames
#  are output into the file 'filename_translations' separated by \001.
#  
sub make_image_file {
    do {
		my $tmp = $latex_body;
		open KC,">imagename_translations" or die "Cannot open filename translation file for writing";
		while ($tmp =~ /\\lthtmlpictureA\{(.*?)\}\%\n\\includegraphics\{(.*?)\}\%/) {
			$tmp = $';
			my $id = $id_map{$1};
			my $oldname = $2;
			$id =~ s/\#.*//;
			print KC "img$id.png\001$oldname\n";
		}
		close KC;

		print "\nWriting image file ...\n";
		open(ENV,">.$dd${PREFIX}images.tex")
				|| die "\nCannot write '${PREFIX}images.tex': $!\n";
		print ENV &make_latex($latex_body);
		print ENV "\n";
		close ENV;
		&copy_file($FILE, "bbl");
		&copy_file($FILE, "aux");
    } if ((%latex_body) && ($latex_body =~ /newpage/));
}


1;  # Must be present as the last line.
