# This module does multiple indices, supporting the style of the LaTex 'index'
#  package.

# Version Information:
#  16-Feb-2005 -- Original Creation.  Karl E. Cunningham

#  The order of the \newindex commands in the LaTex source will determine the order in which 
#  the indices are output.


# Globals to handle multiple indices.
my %index_order;
my %indices;


# KEC 2-18-05
# Handles the \newindex command.  This is called if the \newindex command is 
#  encountered in the LaTex source.  Gets the index names and title from the arguments.
#  Saves the name, title, and the order in which the \newindex commands were found.
#  Note that we are called once to handle multiple \newindex commands that are 
#    newline separated.
#  The indices will be generated at the end of the web document in the same order
#    as the \newindex commands were found in the LaTex.
{ my $cnt = 0;
sub do_cmd_newindex {
	my $data = shift;
	# The data is sent to us as fields delimited by their ID #'s.  We extract the 
	#  fields.
	foreach my $line (split("\n",$data)) {
		my @fields = split (/(?:\<\#\d+?\#\>)+/,$line);

		# The index name and title are the second and fourth fields in the data.
		if ($line =~ /^</ or $line =~ /^\\newindex/) {
			my ($indexname,$indextitle) = ($fields[1],$fields[4]);
			$indices{names}{$indexname} = {title => $indextitle,order => $cnt};
			$indices{order}{$cnt} = {title => $indextitle, name => $indexname};
			$cnt++;
		}
	}
}
}


# KEC -- Copied from makeidx.perl and modified to do multiple indices.
# Processes an \index entry from the LaTex file.  
#  Gets the optional argument from the index command, which is the name of the index
#    into which to place the entry.
#  Drops the brackets from the index_name
#  Puts the index entry into the html stream
#  Creates the tokenized index entry (which also saves the index entry info
sub do_real_index {
	local($_) = @_;
	local($pat,$idx_entry,$index_name);
	# catches opt-arg from \index commands for index.sty
	$index_name = &get_next_optional_argument;
	# Drop leading and trailing brackets from the index name.
	$index_name =~ s/^\[|\]$//g;

	$idx_entry = &missing_braces unless (
		(s/$next_pair_pr_rx/$pat=$1;$idx_entry=$2;''/e)
		||(s/$next_pair_rx/$pat=$1;$idx_entry=$2;''/e));

	if (defined $idx_entry and !defined $indices{names}{$index_name}) {
		print STDERR "\nInvalid Index Name: \\index \[$index_name\]\{$idx_entry\}\n";
	}

	$idx_entry = &named_index_entry($pat, $idx_entry,$index_name);
	$idx_entry.$_;
}

# KEC. -- Copied from makeidx.perl, then modified to do multiple indices.
#  Feeds the index entries to the output. This is called for each index to be built.
#
#  Generates a list of lookup keys for index entries, from both %printable_keys
#    and %index keys.
#  Sorts the keys according to index-sorting rules.
#  Removes keys with a 0x01 token. (duplicates?)
#  Builds a string to go to the index file.
#    Adds the index entries to the string if they belong in this index.
#    Keeps track of which index is being worked on, so only the proper entries
#      are included.
#  Places the index just built in to the output at the proper place.
{ my $index_number = 0;
sub add_real_idx {
	print "\nDoing the index ... Index Number $index_number\n";
	local($key, @keys, $next, $index, $old_key, $old_html);
	# RRM, 15.6.96:  index constructed from %printable_key, not %index
	@keys = keys %printable_key;

	while (/$idx_mark/) {
		$index = '';
		# include non- makeidx  index-entries
		foreach $key (keys %index) {
			next if $printable_key{$key};
			$old_key = $key;
			if ($key =~ s/###(.*)$//) {
				next if $printable_key{$key};
				push (@keys, $key);
				$printable_key{$key} = $key;
				if ($index{$old_key} =~ /HREF="([^"]*)"/i) {
					$old_html = $1;
					$old_html =~ /$dd?([^#\Q$dd\E]*)#/;
					$old_html = $1;
				} else { $old_html = '' }
				$index{$key} = $index{$old_key} . $old_html."</A>\n | ";
			};
		}
		@keys = sort makeidx_keysort @keys;
		@keys = grep(!/\001/, @keys);
		my $cnt = 0;
		foreach $key (@keys) {
			next unless ($index_order{$key} eq $index_number);		# KEC.
			$index .= &add_idx_key($key);
			$cnt++;
		}
		print "$cnt Index Entries Added\n";
		$index = '<DD>'.$index unless ($index =~ /^\s*<D(D|T)>/);
		$index_number++;	# KEC.
		if ($SHORT_INDEX) {
			print "(compact version with Legend)";
			local($num) = ( $index =~ s/\<D/<D/g );
			if ($num > 50 ) {
				s/$idx_mark/$preindex<HR><DL>\n$index\n<\/DL>$preindex/o;
			} else {
				s/$idx_mark/$preindex<HR><DL>\n$index\n<\/DL>/o;
			}
		} else { s/$idx_mark/<DL COMPACT>\n$index\n<\/DL>/o; }
	}
}
}

# KEC.  Copied from latex2html.pl and modified to support multiple indices.
# The bibliography and the index should be treated as separate sections
# in their own HTML files. The \bibliography{} command acts as a sectioning command
# that has the desired effect. But when the bibliography is constructed
# manually using the thebibliography environment, or when using the
# theindex environment it is not possible to use the normal sectioning
# mechanism. This subroutine inserts a \bibliography{} or a dummy
# \textohtmlindex command just before the appropriate environments
# to force sectioning.
sub add_bbl_and_idx_dummy_commands {
	local($id) = $global{'max_id'};

	s/([\\]begin\s*$O\d+$C\s*thebibliography)/$bbl_cnt++; $1/eg;
	## if ($bbl_cnt == 1) {
		s/([\\]begin\s*$O\d+$C\s*thebibliography)/$id++; "\\bibliography$O$id$C$O$id$C $1"/geo;
	#}
	$global{'max_id'} = $id;
	# KEC. Modified to global substitution to place multiple index tokens.
	s/([\\]begin\s*$O\d+$C\s*theindex)/\\textohtmlindex $1/go;
	s/[\\]printindex/\\textohtmlindex /go;
	&lib_add_bbl_and_idx_dummy_commands() if defined(&lib_add_bbl_and_idx_dummy_commands);
}

# KEC.  Copied from latex2html.pl and modified to support multiple indices.
#  For each textohtmlindex mark found, create an index file that includes the 
#  items from the appropriate index. Indices are produced in the same order as
#  the \newindex commands were found in the original LaTex source.  The names for 
#  the index files are created based on the index title if english names are desired.
# $idx_mark will be replaced with the real index at the end
# 
#  One problem is that this routine is called twice..  Once for processing the 
#    command as originally seen, and once for processing the command when 
#    doing the name for the index file. We can detect that by looking at the 
#    br_id numbers surrounding the \theindex command, and not incrementing
#    index_number unless a new br_id is seen.  This has the side effect of 
#    having to unconventionally start the index_number at -1. But it works.
#
#  Gets the title from the list of indices.
#  If this is the first index, save the filename in $first_idx_file. This is what's referenced
#    in the navigation buttons.
#  Increment the index_number for next time.
#  If the indexname command is defined or a newcommand defined for indexname, do it.
#  Save the index TITLE in the toc
#  Save the first_idx_file into the idxfile. This goes into the nav buttons.
#  Build index_labels if needed.
#  Create the index headings and put them in the output stream.
{ my $index_number = -1;  # Will be incremented before use.
	my $first_idx_file;  # Static
	my %ids_done;
sub do_cmd_textohtmlindex {
	local($_) = @_;

	my ($id_no) = /\\begin<<\d+>>theindex<<(\d+)>>/;
	# If we've not seen this id number for 'theindex' before, increment
	#  index_number.
	if (!exists $ids_done{$id_no}) {
		$index_number++;
		$ids_done{$id_no} = undef;
	}
	$idx_title = "Index"; # The name displayed in the nav bar text.

	# Only set $idxfile if we are at the first index.  This will point the navigation panel to the 
	#  first index file rather than the last.
	$first_idx_file = $CURRENT_FILE if ($index_number == 0);
	$idxfile = $first_idx_file; # Pointer for the Index button in the nav bar.
	$toc_sec_title = $indices{order}{$index_number}{title}; # Index link text in the toc.
	$TITLE = $toc_sec_title; # Title for this index, from which its filename is built.
	if (%index_labels) { &make_index_labels(); }
	if (($SHORT_INDEX) && (%index_segment)) { &make_preindex(); }
		else { $preindex = ''; }
	local $idx_head = $section_headings{'textohtmlindex'};
	local($heading) = join(''
		, &make_section_heading($TITLE, $idx_head)
		, $idx_mark );
	local($pre,$post) = &minimize_open_tags($heading);
	join('',"<BR>\n" , $pre, $_);
}
}

# Returns an index key, given the key passed as the first argument.
#  Not modified for multiple indices.
sub add_idx_key {
    local($key) = @_;
	local($index, $next);
	if (($index{$key} eq '@' )&&(!($index_printed{$key}))) {
		if ($SHORT_INDEX) { $index .= "<DD><BR>\n<DT>".&print_key."\n<DD>"; }
			else { $index .= "<DT><DD><BR>\n<DT>".&print_key."\n<DD>"; }
	} elsif (($index{$key})&&(!($index_printed{$key}))) {
		if ($SHORT_INDEX) {
			$next = "<DD>".&print_key."\n : ". &print_idx_links;
		} else {
			$next = "<DT>".&print_key."\n<DD>". &print_idx_links;
		}
		$index .= $next."\n";
		$index_printed{$key} = 1;
	}

	if ($sub_index{$key}) {
		local($subkey, @subkeys, $subnext, $subindex);
		@subkeys = sort(split("\004", $sub_index{$key}));
		if ($SHORT_INDEX) {
			$index .= "<DD>".&print_key  unless $index_printed{$key};
			$index .= "<DL>\n"; 
		} else {
			$index .= "<DT>".&print_key."\n<DD>"  unless $index_printed{$key};
			$index .= "<DL COMPACT>\n"; 
		}
		foreach $subkey (@subkeys) {
			$index .= &add_sub_idx_key($subkey) unless ($index_printed{$subkey});
		}
		$index .= "</DL>\n";
	}
	return $index;
}



# Creates and saves an index entry in the index hashes.
#  Modified to do multiple indices. 
#    Creates an index_key that allows index entries to have the same characteristics but be in
#      different indices. This index_key is the regular key with the index name appended.
#    Save the index order for the entry in the %index_order hash.
sub named_index_entry {
    local($br_id, $str, $index_name) = @_;
	my ($index_key);
    # escape the quoting etc characters
    # ! -> \001
    # @ -> \002
    # | -> \003
    $* = 1; $str =~ s/\n\s*/ /g; $* = 0; # remove any newlines
    # protect \001 occurring with images
    $str =~ s/\001/\016/g;			# 0x1 to 0xF
    $str =~ s/\\\\/\011/g; 			# Double backslash -> 0xB
    $str =~ s/\\;SPMquot;/\012/g; 	# \;SPMquot; -> 0xC
    $str =~ s/;SPMquot;!/\013/g; 	# ;SPMquot; -> 0xD
    $str =~ s/!/\001/g; 			# Exclamation point -> 0x1
    $str =~ s/\013/!/g; 			# 0xD -> Exclaimation point
    $str =~ s/;SPMquot;@/\015/g; 	# ;SPMquot;@ to 0xF
    $str =~ s/@/\002/g; 			# At sign -> 0x2
    $str =~ s/\015/@/g; 			# 0xF to At sign
    $str =~ s/;SPMquot;\|/\017/g; 	# ;SMPquot;| to 0x11
    $str =~ s/\|/\003/g; 			# Vertical line to 0x3
    $str =~ s/\017/|/g; 			# 0x11 to vertical line
    $str =~ s/;SPMquot;(.)/\1/g; 	# ;SPMquot; -> whatever the next character is
    $str =~ s/\012/;SPMquot;/g; 	# 0x12 to ;SPMquot;
    $str =~ s/\011/\\\\/g; 			# 0x11 to double backslash

    local($key_part, $pageref) = split("\003", $str, 2);
    local(@keys) = split("\001", $key_part);
#print STDERR "\nINDEX2 ($str)\n($key_part, $pageref)(@keys)\n";
    # If TITLE is not yet available use $before.
    $TITLE = $saved_title if (($saved_title)&&(!($TITLE)||($TITLE eq $default_title)));
    $TITLE = $before unless $TITLE;
    # Save the reference
    local($words) = '';
    if ($SHOW_SECTION_NUMBERS) { $words = &make_idxnum; }
		elsif ($SHORT_INDEX) { $words = &make_shortidxname; }
		else { $words = &make_idxname; }
    local($super_key) = '';
    local($sort_key, $printable_key, $cur_key);
    foreach $key (@keys) {
		$key =~ s/\016/\001/g; # revert protected \001s
		($sort_key, $printable_key) = split("\002", $key);
		#
		# RRM:  16 May 1996
		# any \label in the printable-key will have already
		# created a label where the \index occurred.
		# This has to be removed, so that the desired label 
		# will be found on the Index page instead. 
		#
		if ($printable_key =~ /tex2html_anchor_mark/ ) {
			$printable_key =~ s/><tex2html_anchor_mark><\/A><A//g;
			local($tmpA,$tmpB) = split("NAME=\"", $printable_key);
			($tmpA,$tmpB) = split("\"", $tmpB);
			$ref_files{$tmpA}='';
			$index_labels{$tmpA} = 1;
		}
		#
		# resolve and clean-up the hyperlink index-entries 
		# so they can be saved in an  index.pl  file
		#
		if ($printable_key =~ /$cross_ref_mark/ ) {
			local($label,$id,$ref_label);
	#	    $printable_key =~ s/$cross_ref_mark#(\w+)#(\w+)>$cross_ref_mark/
			$printable_key =~ s/$cross_ref_mark#([^#]+)#([^>]+)>$cross_ref_mark/
				do { ($label,$id) = ($1,$2);
				$ref_label = $external_labels{$label} unless
				($ref_label = $ref_files{$label});
				'"' . "$ref_label#$label" . '">' .
				&get_ref_mark($label,$id)}
			/geo;
		}
		$printable_key =~ s/<\#[^\#>]*\#>//go;
		#RRM
		# recognise \char combinations, for a \backslash
		#
		$printable_key =~ s/\&\#;\'134/\\/g;		# restore \\s
		$printable_key =~ s/\&\#;\`<BR> /\\/g;	#  ditto
		$printable_key =~ s/\&\#;*SPMquot;92/\\/g;	#  ditto
		#
		#	$sort_key .= "@$printable_key" if !($printable_key);	# RRM
		$sort_key .= "@$printable_key" if !($sort_key);	# RRM
		$sort_key =~ tr/A-Z/a-z/;
		if ($super_key) {
			$cur_key = $super_key . "\001" . $sort_key;
			$sub_index{$super_key} .= $cur_key . "\004";
		} else {
			$cur_key = $sort_key;
		}

		# Append the $index_name to the current key with a \002 delimiter.  This will 
		#  allow the same index entry to appear in more than one index.
		$index_key = $cur_key . "\002$index_name";

		# If the index is valid (already defined by a \newindex command, save the entry.
		#  Otherwise save the entry in the first index.
		if (defined $indices{names}{$index_name}) {
			$index_order{$index_key} = $indices{names}{$index_name}{order}; 
		} else {
			$index_order{$index_key} = 0;
		}

		$index{$index_key} .= ""; 

		#
		# RRM,  15 June 1996
		# if there is no printable key, but one is known from
		# a previous index-entry, then use it.
		#
		if (!($printable_key) && ($printable_key{$index_key}))
			{ $printable_key = $printable_key{$index_key}; } 
#		if (!($printable_key) && ($printable_key{$cur_key}))
#			{ $printable_key = $printable_key{$cur_key}; } 
		#
		# do not overwrite the printable_key if it contains an anchor
		#
		if (!($printable_key{$index_key} =~ /tex2html_anchor_mark/ ))
			{ $printable_key{$index_key} = $printable_key || $key; }
#		if (!($printable_key{$cur_key} =~ /tex2html_anchor_mark/ ))
#			{ $printable_key{$cur_key} = $printable_key || $key; }

		$super_key = $cur_key;
    }
	#
	# RRM
	# page-ranges, from  |(  and  |)  and  |see
	#
    if ($pageref) {
		if ($pageref eq "\(" ) { 
			$pageref = ''; 
			$next .= " from ";
		} elsif ($pageref eq "\)" ) { 
			$pageref = ''; 
			local($next) = $index{$index_key};
#			local($next) = $index{$cur_key};
	#	    $next =~ s/[\|] *$//;
			$next =~ s/(\n )?\| $//;
			$index{$index_key} = "$next to ";
#			$index{$cur_key} = "$next to ";
		}
    }

    if ($pageref) {
		$pageref =~ s/\s*$//g;	# remove trailing spaces
		if (!$pageref) { $pageref = ' ' }
		$pageref =~ s/see/<i>see <\/i> /g;
		#
		# RRM:  27 Dec 1996
		# check if $pageref corresponds to a style command.
		# If so, apply it to the $words.
		#
		local($tmp) = "do_cmd_$pageref";
		if (defined &$tmp) {
			$words = &$tmp("<#0#>$words<#0#>");
			$words =~ s/<\#[^\#]*\#>//go;
			$pageref = '';
		}
    }
	#
	# RRM:  25 May 1996
	# any \label in the pageref section will have already
	# created a label where the \index occurred.
	# This has to be removed, so that the desired label 
	# will be found on the Index page instead. 
	#
    if ($pageref) {
		if ($pageref =~ /tex2html_anchor_mark/ ) {
			$pageref =~ s/><tex2html_anchor_mark><\/A><A//g;
			local($tmpA,$tmpB) = split("NAME=\"", $pageref);
			($tmpA,$tmpB) = split("\"", $tmpB);
			$ref_files{$tmpA}='';
			$index_labels{$tmpA} = 1;
		}
		#
		# resolve and clean-up any hyperlinks in the page-ref, 
		# so they can be saved in an  index.pl  file
		#
		if ($pageref =~ /$cross_ref_mark/ ) {
			local($label,$id,$ref_label);
			#	    $pageref =~ s/$cross_ref_mark#(\w+)#(\w+)>$cross_ref_mark/
			$pageref =~ s/$cross_ref_mark#([^#]+)#([^>]+)>$cross_ref_mark/
				do { ($label,$id) = ($1,$2);
				$ref_files{$label} = ''; # ???? RRM
				if ($index_labels{$label}) { $ref_label = ''; } 
					else { $ref_label = $external_labels{$label} 
						unless ($ref_label = $ref_files{$label});
				}
			'"' . "$ref_label#$label" . '">' . &get_ref_mark($label,$id)}/geo;
		}
		$pageref =~ s/<\#[^\#>]*\#>//go;

		if ($pageref eq ' ') { $index{$index_key}='@'; }
			else { $index{$index_key} .= $pageref . "\n | "; }
    } else {
		local($thisref) = &make_named_href('',"$CURRENT_FILE#$br_id",$words);
		$thisref =~ s/\n//g;
		$index{$index_key} .= $thisref."\n | ";
    }
	#print "\nREF: $sort_key : $index_key :$index{$index_key}";
 
	#join('',"<A NAME=$br_id>$anchor_invisible_mark<\/A>",$_);

    "<A NAME=\"$br_id\">$anchor_invisible_mark<\/A>";
}

1;  # Must be present as the last line.
