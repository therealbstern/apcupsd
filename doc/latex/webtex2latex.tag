#
#
# webtex2latex.tag  7/22/97
#
# This file attempts to change webtex to latex by processing each 
# subsequent command (consisting of `\` followed by letters)
#

# $webtexCommand{""}=; 
$webtexCommand{"\\centerdot"}='\\cdot'; 
$webtexCommand{"\\hkarrow"}='\\hookrightarrow'; 
$webtexCommand{"\\gt"}='>'; 
$webtexCommand{"\\iiint"}='\\int\\!\\!\\int\\!\\!\\int'; 
$webtexCommand{"\\iint"}='\\int\\!\\!\\int'; 
$webtexCommand{"\\implies"}='\\;\\Longrightarrow\\;'; 
$webtexCommand{"\\llarrow"}='\\longleftarrow'; 
$webtexCommand{"\\lrarrow"}='\\longrightarrow'; 
$webtexCommand{"\\lt"}='<'; 
$webtexCommand{"\\ncong"}='\\not\\cong'; 
$webtexCommand{"\\nexists"}='\\not\\!\\exists'; 
$webtexCommand{"\\ngeq"}='\\not\\geq'; 
$webtexCommand{"\\ngtr"}='\\not>'; 
$webtexCommand{"\\nleq"}='\\not\\leq'; 
$webtexCommand{"\\nless"}='\\not<'; 
$webtexCommand{"\\nmid"}='\\not\\:\\mid'; 
$webtexCommand{"\\nparallel"}='\\not\\,\\parallel'; 
$webtexCommand{"\\nsubset"}='\\not\\subset'; 
$webtexCommand{"\\nsubseteq"}='\\not\\subseteq'; 
$webtexCommand{"\\nsupset"}='\\not\\supset'; 
$webtexCommand{"\\nsupseteq"}='\\not\\supseteq'; 
$webtexCommand{"\\smallsetminus"}='\\raisebox{.5ex}{$\\scriptscriptstyle \\setminus$}'; 

    # Spaces
$webtexCommand{"\\thinsp"}='\\,'; 
$webtexCommand{"\\medsp"}='\\:'; 
$webtexCommand{"\\thicksp"}='\\;'; 
$webtexCommand{"\\negsp"}='\\!'; 

    # Wide accents
$webtexCommand{"\\widebar"}='\\overline'; 
$webtexCommand{"\\widevec"}='\\overrightarrow'; 
$webtexCommand{"\\widecheck"}='\\check';

    # Variable size operators :  These may no longer need to be replaced
    #                            in the new version of WebTeX.
$webtexCommand{"\\otimes"}='\\bigotimes'; 
$webtexCommand{"\\oplus"}='\\bigoplus'; 
$webtexCommand{"\\cup"}='\\bigcup'; 
$webtexCommand{"\\cap"}='\\bigcap'; 
$webtexCommand{"\\wedge"}='\\bigwedge'; 

    # These variable size operators must always be large
    # This may not work with subscripts; check it!
$webtexCommand{"\\bigotimes"}='{\\displaystyle \\bigotimes}'; 
$webtexCommand{"\\bigoplus"}='{\\displaystyle \\bigoplus}'; 
$webtexCommand{"\\bigcup"}='{\\displaystyle \\bigcup}'; 
$webtexCommand{"\\bigcap"}='{\\displaystyle \\bigcap}'; 
$webtexCommand{"\\bigwedge"}='{\\displaystyle \\bigwedge}'; 

    # Text boxes
$webtexCommand{"\\text"}='\\mbox'; 

    # Rendering mode and font sizes
    # We must put the commands inside the braces
$webtexCommand{"\\textsize"}='&webtexChangeToStyle'; 
$webtexCommand{"\\scriptsize"}='&webtexChangeToStyle'; 
$webtexCommand{"\\scriptscriptsize"}='&webtexChangeToStyle'; 
$webtexCommand{"\\displaystyle"}='&webtexSwitchWithBrace'; 
$webtexCommand{"\\textstyle"}='&webtexSwitchWithBrace'; 

$webtexCommand{"\\root"}='&webtexROOT';
$webtexCommand{"\\binom"}='&webtexBINOM';
$webtexCommand{"\\rule"}='&webtexRULE';

    # These commands and some of the arguments are removed, leaving
    # only the argument you see on the screen.
$webtexCommand{"\\href"}='&webtexSecondArgument';
$webtexCommand{"\\statusline"}='&webtexSecondArgument';
$webtexCommand{"\\fghilight"}='&webtexSecondArgument';
$webtexCommand{"\\bghilight"}='&webtexSecondArgument';
$webtexCommand{"\\fontcolor"}='&webtexSecondArgument';
$webtexCommand{"\\toggle"}='&webtexFirstOfFourArguments';

$webtexCommand{"\\array"}='&webtexARRAY';

$webtexCommand{"\\mathop"}="&webtexSurroundArgWith('\\mbox{','}')";

$webtexCommand{"\\overset"}="\\htmlOVERSET";
$webtexCommand{"\\underset"}="\\htmlUNDERSET";

$webtexCommand{"\\left"}='&webtexLeftRightDelim';
$webtexCommand{"\\right"}='&webtexLeftRightDelim';

$webtexCommand{"\\mathfr"}="\\mathfrak";

$webtexCommand{"\\ulap"}='&webtexRemoveCommand';
$webtexCommand{"\\dlap"}='&webtexRemoveCommand';
$webtexCommand{"\\rlap"}='&webtexAddDollarSigns';
$webtexCommand{"\\llap"}='&webtexAddDollarSigns';

$webtexCommand{"\\space"}="&webtexSPACE";

$webtexCommand{"\\tensor"} = '&webtexTENSOR';
$webtexCommand{"\\multiscripts"} = '&webtexTENSOR';



#
#  &webtexParse2Latex  - Convert the webtex commands in the input string
#                        to LaTeX
#
#  While there is a '\' command in the remaining buffer
#    Set the buffer to be whatever follows it
#    Add to Output the stuff that preceeds it
#    Process the webtex command
#    Add the (perhaps) revised command to the output
#  Add any remaining text to output and return
#  
sub webtexParse2Latex
{
    local ($webtexBuffer) = @_;
    local ($output) = '';
    local ($webtexCommandIn,$webtexCommandOut) = ('','');

    while ($webtexBuffer =~ m/\\[a-zA-Z]+/)
    {
	$webtexCommandOut = '';
	$webtexBuffer = $';
	$webtexCommandIn = $&;
	$output .= $`;
	&webtexHandleCommand;
	$output .= $webtexCommandOut;
    }
    $output .= $webtexBuffer;
    
    return $output;
}

#



#
#  &webtexHandleCommand
#
#    If the tag is defined, do what it says, otherwise send it back verbatim
#
sub webtexHandleCommand
{

    if (defined($webtexCommand{$webtexCommandIn})) 
    {&webtexDoString($webtexCommand{$webtexCommandIn})}
    else {$webtexCommandOut = $webtexCommandIn;}

}


#
#  &webtexDoString  - execute or print a string
#
#  If the string begins with "&"
#    Evaluate the string as a perl command and report any errors
#  Otherwise the string is the webtex command that will be printed
#
sub webtexDoString
{
  local ($string) = @_;
  if (substr($string,0,1) eq "&") { eval $string; warn $@ if ($@)}
    else {$webtexCommandOut =  $string;}

}




#
# &webtexSwitchWithBrace
#
# This subroutine takes the opening brace to the right of the command
# and moves it to the left of the command.
# This is necessary for commands such as \textstyle, which is changed
# to the \textstyle mode in LaTeX.
#
# Add a { to the left side of the command.
# Replace an initial { in the buffer with a space.
#
sub webtexSwitchWithBrace
{
    $webtexCommandOut = '{'.$webtexCommandIn;
    $webtexBuffer =~ s/^\s*\{/ /;

}


#
#&webtexChangeToStyle
#
# Move the brace to the left of the command
# Replace the word 'size' with 'style' 
#
#
sub webtexChangeToStyle
{
    &webtexSwitchWithBrace;
    $webtexCommandOut =~ s/size/style/;

}


#
# &webtexNextArgument
#
# Get the next argument (inside outermost { } ) from the webtex buffer
#
#   LOOP until we reach the outermost right brace
#     read up to next { or }  (ERROR if there are no braces)
#     Check for \{, \} and ignore if true
#     Increase nesting count if it's Left Brace 
#     Decrease nesting count if it's Right Brace
#     Add prev. and brace to output
#     Leave rest in buffer
#   stop when count is 0 
#   return the argument (it's been removed from the buffer)
#
#
sub webtexNextArgument
{
    local ($nest) = 0;
    local ($argument) = '';

    do {

	if ($webtexBuffer =~ /{|}/ == 0) 
	{ 
  &cliError("webtexNextArgument: Ran out of braces before finishing argument")
        }
	$last = substr($`,-1);
	if ($last ne '\\')  {

	    $nest++ if ($& eq '{');
	    $nest-- if ($& eq '}');
	
	    $argument .= $`.$&;
	    $webtexBuffer = $';
	}
    }  until ($nest == 0);	
	
 	return $argument;
	
}

#
# &webtexNextArgument2
#
# Get the next argument (inside outermost { } ) from the from buffer
# that is passed in as the argument.
#
#
#   LOOP until we reach the outermost right brace
#     read up to next { or }  (ERROR if there are no braces)
#     Check for \{, \} and ignore if true
#     Increase nesting count if it's Left Brace 
#     Decrease nesting count if it's Right Brace
#     Add prev. and brace to output
#     Leave rest in buffer
#   stop when count is 0 
#   return the argument and the remaining buffer
#
#
sub webtexNextArgument2
{
    local ($nest) = 0;
    local ($argument) = '';
    local ($buffer) = @_;

    do {
	if ($buffer =~ /{|}/ == 0) 
	{
  &cliError("webtexNextArgument: Ran out of braces before finishing argument")
        }
	$last = substr($`,-1);

	if ($last ne '\\')  {
	    $nest++ if ($& eq '{');
	    $nest-- if ($& eq '}');
       	    $argument .= $`.$&;
	    $buffer = $';
	}
    }  until ($nest == 0);	
	
	return ($argument,$buffer);
	
}



#
# &webtexROOT
#
# The first argument of root is changed to an optional Latex argument
# between brackets [ ]
# The command itself is changed to \sqrt
#
sub webtexROOT
{
    local ($arg);

    $arg = &webtexNextArgument;
    $arg =~ s/^\s*{/[/;
    $arg =~ s/}$/]/;

    $webtexCommandOut = "\\sqrt";
    $webtexBuffer = $arg.$webtexBuffer;
}


#
# &webtexBINOM
#
# The two arguments of binom are read in and
# then placed as entries in a 2x1 array.
# 
# read in the arguments
# remove the braces surrounding the arguments
# construct the array
# put the beginning of the array in the command
# put the rest of the array back in the buffer (in case arguments had
#    webtex commands that need to be parsed)
#
sub webtexBINOM
{
    local ($arg1, $arg2);

    $arg1 = &webtexNextArgument;
    $arg2 = &webtexNextArgument;

    $arg1 =~ s/^\s*{/ /;
    $arg1 =~ s/}$/ /;
    $arg2 =~ s/^\s*{/ /;
    $arg2 =~ s/}$/ /;

    $webtexCommandOut = '\\begin{array}{c} ';
    $webtexBuffer = $arg1.' \\\\ '.$arg2.'\\end{array} '.$webtexBuffer; 
}

#
# webtexRULE
#
# Here we change the three arguments for WebTeX rules to the 
# three arguments required by LaTeX.  The name of the command remains the
# same.  We don't bother to check the arguments for numbers because 
# perl seems to ignore any letters that might be in the arguments.
#
# Get the three arguments.
# Remove the braces.
# ############Check that they are all numbers (error if not)
# Calculate the new arguments
#  
sub webtexRULE
{
    local ($ht, $dp, $wd);
    local ($a, $b, $c);

    $ht = &webtexNextArgument;
    $dp = &webtexNextArgument;
    $wd = &webtexNextArgument;

    $ht =~ s/^\s*{/ /;    $ht =~ s/}$/ /;
    $dp =~ s/^\s*{/ /;    $dp =~ s/}$/ /;
    $wd =~ s/^\s*{/ /;    $wd =~ s/}$/ /;
    
    # if ($ht =~ /^\s*-?\d*.?\d*\s*$/ &&$arg1 =~ /^\s*-?\d*.?\d*\s*$/ &&
    # $arg1 =~ /^\s*-?\d*.?\d*\s*$/)

    $a = -0.1 * $dp;
    $b = 0.1 * $wd;
    $c = 0.1 * ($ht + $dp);

    $webtexCommandOut = "\\rule[$a ex]{$b em}{$c ex}";


}


#
# webtexSPACE
#
# We create a space of the desired size by using an algorithm 
# described in the solution of Exercise 12.5 in The TeXbook by
# Donald Knuth. (pp. 82 and 312)
# 
#
# Get the three arguments.
# Remove the braces.
# Multiply parameters by 1/10 to get tenths of ems and exes.
# Set up the output command with the height, width and depth
# inserted in the correct spots.
#  
sub webtexSPACE
{
    local ($ht, $dp, $wd);

    $ht = &webtexNextArgument;
    $dp = &webtexNextArgument;
    $wd = &webtexNextArgument;

    $ht =~ s/^\s*{/ /;    $ht =~ s/}$/ /;
    $dp =~ s/^\s*{/ /;    $dp =~ s/}$/ /;
    $wd =~ s/^\s*{/ /;    $wd =~ s/}$/ /;
    
    $ht = 0.1*$ht; 
    $dp = 0.1*$dp; 
    $wd = 0.1*$wd; 


    $webtexCommandOut = 
       "{\\setbox0=\\null \\ht0=$ht ex \\wd0=$wd em \\dp0=$dp ex \\box0 }";


}


#
# &webtexSecondArgument
#
# For the actions such as \href, \statusline, etc. we throw out the command 
# and the first argument, leaving only the second argument as it is in 
# the buffer.
#
# Remove the first argument (It's never heard from again.)
# The command is set to null.
#
sub webtexSecondArgument
{
    local ($arg);

    $arg = &webtexNextArgument;

    $webtexCommandOut = '';
    
}



#
# &webtexFirstOfFourArguments
#
# For the toggle command, we throw out the command 
# and the second through fourth arguments, 
# leaving only the first argument as it is in 
# the buffer.
#
# Save the first argument 
# Remove the second, third, and fourth arguments 
# Put the first argument back in the buffer.
# The command is also set to null.
#
sub webtexFirstOfFourArguments
{
    local ($arg1,$arg2);

    $arg1 = &webtexNextArgument;

    $arg2 = &webtexNextArgument;

    $arg2 = &webtexNextArgument;

    $arg2 = &webtexNextArgument;

    $webtexBuffer = $arg1.$webtexBuffer;

    $webtexCommandOut = '';
    
}

#
# &webtexRemoveCommand
#
# This subroutine gets rid of a command and the braces around its
# argument.  This is appropriate only for commands with one argument.
#
#
# Read in the argument.
# Remove the braces around the argument.
# Put the argument back in the buffer.
# Set the command to null.
#
#
sub webtexRemoveCommand
{
    local ($arg1);

    $arg1 = &webtexNextArgument;

    $arg1 =~ s/^\s*{/ /;
    $arg1 =~ s/}$/ /;

    $webtexBuffer = $arg1.$webtexBuffer;

    $webtexCommandOut = '';
    
}


#
# &webtexAddDollarSigns
#
# Surround the argument with a pair of dollar signs.
# 
#
# Read in the argument.
# Put in dollar signs just inside braces
# Put the argument back in the buffer.
# Set the command to null.
#
#
sub webtexAddDollarSigns
{
    local ($arg1);

    $arg1 = &webtexNextArgument;

    $arg1 =~ s/^\s*{/{\$/;
    $arg1 =~ s/}$/\$}/;

    $webtexBuffer = $arg1.$webtexBuffer;

    $webtexCommandOut = $webtexCommandIn;
    
}


#
# &webtexARRAY
#
# Convert a webtex array to a latex array in array mode.
#
# Get the argument; that is the array itself.
# Remove outer braces 
# Check for arrayopts.
#   Is first command in array \arrayopts?
#   If so, read in the options; split on \ to get each option
#   Switch to decide what to do with each option.
#     COLLAYOUT:  split argument by spaces;
#                 put l,c,r,| in column format
#     ALIGN:      center: ignore
#                 top,bottom: set vert. pos. to [t] or [b]
#                 r#n: find no. of rows, 
#     PADDING:    Put together arraystretch and extracolsep strings
#     EQUALROWS, EQUALCOLS: ignore for now.
#
# Split array at & and \\
# LOOP: each entry in split array
#   If it's the beginning of a row, Look for \rowopts
#     UNDERLINE:  put '\hline' at the end of the row ('\\ \hline' if last row)
#     ALIGN:      ignore
#
#   Look for \cellopts
#     COLSPAN:    create a \multicolumn
#     ROWSPAN:    ignore
#     ROWALIGN: ignore
#     COLALIGN: Put in an \hfill that pushes the entry to the right or left
# If necessary, find out the number of columns and rows in this array.
# Fill column format if necessary
#   Also combine the COLALIGN and COLLINES strings into COLLAYOUT
# Finish alignment by row if necessary.
#
# Set up the command and buffer strings

sub webtexARRAY
{
    local ($array, $depth, $numcols,$entry,$element)= ('',0,0,'','');
    local ($arrayoutput) = '';
    local ($padding1,$padding2,$padding3,$align1,$align2,$align3)
	= ('','','','','','');
    local ($collayout, $colspan1,$colspan2)
	= ('','','');
    local ($rowlines,$collines,$colalign,$loc_colalign) = ('','','','');
    local ($leftfill, $rightfill) = ('','');
    local ($rownum) = 0;
#   local ($multicolctr) = 0;

# Get the argument; that is the array itself.
    $array = &webtexNextArgument;

# Remove outer braces / replace the last one with a '&'
    $array =~ s/^\s*{/ /;    $array =~ s/}$/&/;

# Check for arrayopts.
#   Is first command in array \arrayopts?
#      Reset $array to get rid of \arrayopts command
#      Get the argument of \arrayopts
#      Parse the options  (this will set some variables)
    if ($array =~ /^\s*\\arrayopts/) 
    {
	$array = $';
        ($arrayargs,$array) = &webtexNextArgument2($array);  
        &webtexParseArrayOpts($arrayargs);
    }
#
# Split array at & and \\, keeping these symbols for reference
    @elements = split(/(&|\\\\)/, $array);

# LOOP: each entry in split array  
    foreach $element (@elements) {
	if ($element ne '&' && $element ne '\\\\' || $depth > 0)
	{
	    $depth += &webtexDepth($element); 
	    $entry .= $element;
	}
	# Ignore &'s for a time after multicolumn statements.
#	elsif ($multicolctr > 0 && $element eq '&') { 
#	    $numcols++;$multicolctr--;
#	} 

	else {
#   If it's the beginning of a row, Look for \rowopts
#     UNDERLINE:  put '\hline' at the end of the row ('\\ \hline' if last row)
#     ALIGN:      ignore
	    if ($entry =~ /^\s*\\rowopts/) 
	    {
		$entry = $';
		($rowargs,$entry) = &webtexNextArgument2($entry); 
		&webtexParseRowOpts($rowargs);
	    }		    
### -------------------------------------------
# Take care of \rowopts--defined colaligns.

	    if ($loc_colalign ne '') {
		if ($numcols < length($loc_colalign)) {
		    $position = substr($loc_colalign,$numcols,1);
		}
		else {
		    $position = substr($loc_colalign,-1);
		}
		
		if ($position eq 'l' || $position eq 'c') {
		    $rightfill = '\\hfill ';
		}
		if ($position eq 'r' || $position eq 'c') {
		    $leftfill = '\\hfill ';
		}
							 

	    }



### -------------------------------------------


#
#   Look for \cellopts
#     COLSPAN:    create a \multicolumn
#     ROWSPAN:    ignore
#     ROWALIGN, COLALIGN: ignore

	    if ($entry =~ /^\s*\\cellopts/) 
	    {
		$entry = $';
		($cellargs,$entry) = &webtexNextArgument2($entry);
		&webtexParseCellOpts($cellargs);
	    }		    

	    # if $multicolctr>0, we must remove the current '&'
#	    if ($multicolctr > 0 && $element eq '&') 
#obsolete.  { 
#		$element = '';
#		$multicolctr--;
#	    }

           $arrayoutput .= $leftfill.$colspan1.$entry.$colspan2.$rightfill.' '.$element.' ';
	    


	    if ($element eq '\\\\') { 
		$rownum++;
		if ($rownum  <= length($rowlines) ) {
		    if (substr($rowlines,$rownum-1,1) eq 'y') {
			$arrayoutput .= '\\hline ';
		    }
		}
		elsif (substr($rowlines,-1) eq 'y') {
		    $arrayoutput .= '\\hline ';
		}
		# These var. are reset after each row.
		$numcols = 0;
		$loc_colalign = '';
		# if $multicolctr>0 and we get to \\, we must reset
#		$multicolctr = 0;
	    }
	    else { $numcols++; }   # $element = `&`

	    $colspan1 = ''; 
	    $colspan2 = '';
	    $entry = '';
	    $element = '';
	    $leftfill = '';
	    $rightfill = '';
	}
    }

# Remove the last two characters of the $arrayoutput, since 
# they will be a terminal '& ' which was only added to force the
# printing of the last entry.
    substr($arrayoutput, -2) = '  ';

##------ Perhaps this could be made more efficient -------------
# If necessary, find out the number of columns and rows in this array.
# Fill column format if necessary
    if ($collayout eq '') { 
#	$collayout = 'c' x $numcols; 

#   If colalign is empty, put all c's
	if ($colalign eq '') {$colalign = 'c' x $numcols; }
#   Fill alignments with repeat of last characters 
	else
	{  $colalign .= substr($colalign,-1) x ($numcols - length($colalign));}

	if ($collines ne '')
	{  $collines .= substr($collines,-1) x
                          ($numcols - length($collines) - 1);
	   #Weave the two variables together here.
	   for $count (0..$numcols-2) {
	       $collayout .= 
		   substr($colalign,$count,1).substr($collines,$count,1);
	   }
	   $collayout .= substr($colalign,-1);
	   $collayout =~ s/n//g; #remove indicators of no lines		       
       }
	else {$collayout = $colalign;}
	   
}
##---------------------------------------------------------------


# Finish alignment by row if necessary.  (Option r#n needs this.)
#
# Set up the command and buffer strings

 #   $webtexCommandIn = '';
    $webtexCommandOut = 	
        "$padding1 $align1 \\begin\{array\}$align2\{$padding2"."$collayout\}";

    $webtexBuffer =
 	$arrayoutput."\\end\{array\} ".$align3.$padding3.$webtexBuffer;

}



# 
# &webtexParseArrayOpts
#
# Changes the values of $collayout, $align2, $padding1, $padding2
#
#   read in the options; split on \ to get each option
#   Switch to decide what to do with each option.
#     COLLAYOUT:  split argument by spaces;
#                 put l,c,r,| in column format
#     COLALIGN:   split argument by spaces;
#                 put l,c,r in column format
#     COLLINES:     split argument by spaces;
#                 put | or n in lines format (n's will be removed later)
#     ALIGN:      center: ignore
#                 top,bottom: set vert. pos. to [t] or [b]
#                 r#n: find no. of rows, 
#     PADDING:    Put together arraystretch and extracolsep strings
#     EQUALROWS, EQUALCOLS: ignore for now.
#

sub webtexParseArrayOpts
{
#   read in the options; split on \ to get each option
	local ($options) = @_;
	@options = split(/\\/,$options);
	
	foreach $opt (@options) {
#   Switch to decide what to do with each option.
#     COLLAYOUT:  split argument by spaces;
#                 put l,c,r,| in column format
	    if ($opt =~ /collayout/) {
		$collayout = '';
		@layout = split(/\s+/,$opt);
		foreach $col (@layout) {
		  SWITCH: {
		      if ($col =~ /right/i) { $collayout .= 'r'; 
				      last SWITCH; }  
		      if ($col =~ /center/i) { $collayout .= 'c'; 
				      last SWITCH; }  
		      if ($col =~ /left/i) { $collayout .= 'l'; 
				      last SWITCH; }  
		      if ($col =~ /solid/i) { $collayout .= '|'; 
				      last SWITCH; }  
		      if ($col =~ /dashed/i) { $collayout .= '|'; 
				      last SWITCH; }  
	  
		  }
		}
	    }

#     COLALIGN:  split argument by spaces;
#                 put l,c,r in column format
	    if ($opt =~ /colalign/) {
		$colalign = '';
		@layout = split(/\s+/,$opt);
		foreach $col (@layout) {
		  SWITCH: {
		      if ($col =~ /right/i) { $colalign .= 'r'; 
				      last SWITCH; }  
		      if ($col =~ /center/i) { $colalign .= 'c'; 
				      last SWITCH; }  
		      if ($col =~ /left/i) { $colalign .= 'l'; 
				      last SWITCH; }  
	  
		  }
		}
	    }

#     COLLINES:  split argument by spaces;
#                 put | for solid/dashed, n for none
	    if ($opt =~ /collines/) {
		$collines = '';
		@layout = split(/\s+/,$opt);
		foreach $col (@layout) {
		  SWITCH: {
		      if ($col =~ /solid|dashed/i) { $collines .= '|'; 
				      last SWITCH; }  
		      if ($col =~ /none/i) { $collines .= 'n'; 
				      last SWITCH; }  
	  
		  }
		}
	    }


#     ROWLINES:  split argument by spaces;
#                 put y for solid/dashed, n for none
	    if ($opt =~ /rowlines/) {
		$rowlines = '';
		@layout = split(/\s+/,$opt);
		foreach $row (@layout) {
		  SWITCH: {
		      if ($row =~ /solid|dashed/i) { $rowlines .= 'y'; 
				      last SWITCH; }  
		      if ($row =~ /none/i) { $rowlines .= 'n'; 
				      last SWITCH; }  
	  
		  }
		}
	    }

#     ALIGN:      center: ignore
#                 top,bottom: set vert. pos. to [t] or [b]
#                 r#n: find no. of rows, 
	    elsif ($opt =~ /align/) {
		  SWITCH: {
		      if ($opt =~ /top/i) { $align2 = '[t]'; 
				      last SWITCH; }  
		      if ($opt =~ /center/i) { $align2 = ''; 
				      last SWITCH; }  
		      if ($opt =~ /bottom/i) { $align2 = '[b]'; 
				      last SWITCH; }  
		  }
		}
#     PADDING:    Put together arraystretch and extracolsep strings
	    elsif ($opt =~ /padding/) {
		if ($opt =~ /\d+/) {
		    $padamt = $&;
		    $padding1 = "\\renewcommand\{\\arraystretch\}\{$padamt\}";
		    $padding2 = "@\{\\extracolsep\{$padamt\\arraycolsep\}\}";
		}
	    }
		
#     ROWSPACING:    Put together arraystretch string
	    elsif ($opt =~ /rowspacing/) {
		if ($opt =~ /\d+/) {
		    $padamt = $&;
		    $padding1 = "\\renewcommand\{\\arraystretch\}\{$padamt\}";
		}
	    }
		
#     COLUMNSPACING:    Put together extracolsep strings
	    elsif ($opt =~ /columnspacing/) {
		if ($opt =~ /\d+/) {
		    $padamt = $&;
		    $padding2 = "@\{\\extracolsep\{$padamt\\arraycolsep\}\}";
		}
	    }
		
#     EQUALROWS, EQUALCOLS: ignore for now.

	}

}

#
#  &webtexParseRowOpts
#
#  Check arguments of \rowopts
#     UNDERLINE:  If the argument of \underline is 'solid' or 'dashed' 
#                 put '\hline' at the end of the row ('\\ \hline' if last row)
#     ALIGN:      ignore

sub webtexParseRowOpts
{
#   read in the options; split on \ to get each option
	local ($options) = @_;
	@options = split(/\\/,$options);
	
	foreach $opt (@options) {
#   Switch to decide what to do with each option.

#     COLALIGN:  split argument by spaces;
#                 put l,c,r in column format
	    if ($opt =~ /colalign/) {
		$colalign = '';
		@layout = split(/\s+/,$opt);
		foreach $col (@layout) {
		  SWITCH: {
		      if ($col =~ /right/i) { $loc_colalign .= 'r'; 
				      last SWITCH; }  
		      if ($col =~ /center/i) { $loc_colalign .= 'c'; 
				      last SWITCH; }  
		      if ($col =~ /left/i) { $loc_colalign .= 'l'; 
				      last SWITCH; }  
	  
		  }
		}
	    }
#     ALIGN:     ignore

	}

}

#
#  &webtexParseCellOpts
#
#   Look for \colopts
#     COLSPAN:    create a \multicolumn
#         BUG: WebTeX requires the extra &'s that correspond to the
#              columns that are being written over.  These remain in
#              the output, even though LaTeX does not accept them.
#     ROWSPAN:    ignore
#     ROWALIGN, COLALIGN: ignore

sub webtexParseCellOpts
{
#   read in the options; split on \ to get each option
	local ($options) = @_;
	@options = split(/\\/,$options);
	
	foreach $opt (@options) {
#   Switch to decide what to do with each option.
	    # COLSPAN: we create a \multicolumn
	    if ($opt =~ /colspan/) {
		if ($opt =~ /\d+/) {
		    $cspanamt = $&;
		    $colspan1 = "\\multicolumn{$cspanamt}{c}{";
		    $colspan2 = "}";
# We must keep track of the number N of columns that will be spanned and
# ignore N-1 &'s.
#  No longer	    $colspan1 =~ /(\d+)/;
#  necessary.	    $multicolctr = $1 - 1;    
		}
	    }
	

	    elsif ($opt =~ /colalign/) {
		  SWITCH: {
		      if ($opt =~ /left/i) { 
			  $rightfill = '\\hfill ';
			  $leftfill = '';
				      last SWITCH; }  
		      if ($opt =~ /center/i) { 
			  $rightfill = '\\hfill ';
			  $leftfill = '\\hfill ';
				      last SWITCH; }  
		      if ($opt =~ /right/i) { 
			  $rightfill = '';
			  $leftfill = '\\hfill ';
				      last SWITCH; }  
		  }
		
	    }

#     ROWSPAN:     ignore
#     ROWALIGN: ignore

	}

# If both colspan and colalign are set, we should consolidate them
# by changing the positioning argument of \multicolumn and forgetting
# about the \hfill commands.

	if ($colspan2 ne '') {
	    if ($leftfill eq '' && $rightfill eq '\\hfill ')
	       {$colspan1 =~ s/{c}/{l}/;}

	    if ($leftfill eq '\\hfill ' && $rightfill eq '' )
	       {$colspan1 =~ s/{c}/{r}/;}

	    $rightfill = '';
	    $leftfill = '';
	}

}


#
#  &webtexDepth
#
#  Measure the number of left braces minus the number of right braces in
# the argument string.  This indicates the depth of the nesting of braces.
#
# Count the number of left and right braces.
# Count the number of \{ and \}, which are for display only.
# Subtract to get the depth in this string.
# return the depth



sub webtexDepth
{

    local ($str) = @_;

    $left = ($str =~ s/\{/{/g);
    $right = ($str =~ s/\}/}/g);
#    $depth = $left - $right;

    $lescaped = ($str =~ s/(\\\{)/$1/g);
    $rescaped = ($str =~ s/(\\\})/$1/g);
    $ileft = $left - $lescaped;
    $iright =$right - $rescaped;
    $idepth = $ileft - $iright;
    
    return $idepth;
}


sub webtexSurroundArgWith
{
    local ($leftpart,$rightpart) = @_;
    local ($nextarg);

    $nextarg = &webtexNextArgument;
    
    $nextarg =~ s/^\s*{/{$leftpart/;
    $nextarg =~ s/}$/$rightpart}/;
    		
    $webtexCommandOut = $webtexCommandIn;
    $webtexBuffer = $nextarg.$webtexBuffer;

}

# For some reason, substituting this variable into the matching test below
# causes extra matches (perhaps with empty strings).
#$webtexDelimiters = 
#    '\\{|\\}|\(|\)|\[|\]|\\lceil|\\rceil|\\lfloor|\\rfloor|\\langle|\\rangle|\\\||\|';

#
#  &webtexLeftRightDelim
#
#  If the commands \left and \right are followed by no delimiters,
#  we must add a dot for LaTeX.
#
#  If the next non-space char in the buffer is not a WebTeX delimeter,
#     set the extra dot flag.
#  Place the command in the output, with the dot if necessary.
#

sub webtexLeftRightDelim
{

    local ($extradot) = '';

if ($webtexBuffer !~ /^\s*(\\{|\\}|\(|\)|\[|\]|\\lceil|\\rceil|\\lfloor|\\rfloor|\\langle|\\rangle|\\\||\|)/)
{
    $extradot = '.';
}

$webtexCommandOut = $webtexCommandIn.$extradot;


}


#
# webtexParseTensor
#
# If this is \multiscripts,
#   Read prescript argument
#   Remove the braces
#   Parse the prescript argument
# Read base argument
# Remove the braces
# Read postscript argument
# Remove the braces
# Parse the postscript argument
#
# Add to the buffer


sub webtexTENSOR
{
    local ($prescripts, $base, $postscripts);
    local ($preOut, $postOut) = ('','');

    if ($webtexCommandIn eq "\\multiscripts") 
    {
	$prescripts = &webtexNextArgument;
	$prescripts =~ s/^\s*{//;
	$prescripts =~ s/}$//;
	$preOut = '{}'.&webtexParseTensorArguments($prescripts);
    }

    $base = &webtexNextArgument;
    $postscripts = &webtexNextArgument;

    $base =~ s/^\s*{//;
    $base =~ s/}$//;

    $postscripts =~ s/^\s*{//;
    $postscripts =~ s/}$//;

    $postOut = &webtexParseTensorArguments($postscripts);

    $webtexBuffer = $preOut.$base.$postOut.$webtexBuffer;


}

# 
# webtexParseTensorArguments
# -- This outline doesn't quite match the code. --
#
# Check the next character
# If it's {, then read up to corresponding } and send to output.
# ELSE If it's ^
#   Was the last one a ^ ?
#     Add {} to output
#     Set finished_pair to false.
#   Else (_)
#     Was there a ^_ pair?
#       Add {} to output
#       Set finished_pair to false.
#     Else set finished_pair to true.
#   Send ^ and others to output.
#   Set last to ^
#       
# ELSE If it's _
#   Was the last one a _ ?
#     Add {} to output
#     Set finished_pair to false.
#   Else (^)
#     Was there a ^_ pair?
#       Add {} to output
#       Set finished_pair to false.
#     Else 
#   Send _ and others to output.
#   Set last to _
#       
# ELSE read up to next ^ _ or { and send to output
#
# When we have run out of characters
# return output
#



sub webtexParseTensorArguments
{
    local ($scripts) = @_;
    local ($output, $lastScript, $nextchar) = ('','','');
    local ($arg, $finished_pair) = ('','true');

    while ($scripts =~ /[\^\_\{]/) {
	
        $output .= $`;
	$nextchar = $&;
	$scripts = $';
	
	if ($nextchar eq '{') {
	    ($arg,$scripts) = &webtexNextArgument2('{'.$scripts);
	    $output .= $arg; 
	}
	elsif ($nextchar eq '^') {
	    if ($lastScript eq '^') {
		if ($finished_pair eq 'true') {
		    $output .= '{}';
		    $finished_pair = 'false';
	        }	
		else {
		    $output .= '_{}{}';
		    $finished_pair = 'false';
	    
		}
	    }
	    else {
		if ($finished_pair eq 'true') {
		    $output .= '{}';
		    $finished_pair = 'false';
		}
		else {
		    $finished_pair = 'true';
		}
	    }
	    $output .= '^';
	    $lastScript = '^';
		
	}

	elsif ($nextchar eq '_') {
	    if ($lastScript eq '_') {
		if ($finished_pair eq 'true') {
		    $output .= '{}';
		    $finished_pair = 'false';
		}
		else {
		    $output .= '^{}{}';
		    $finished_pair = 'false';
		}
	    }
	    else {
		if ($finished_pair eq 'true') {
		    $output .= '{}';
		    $finished_pair = 'false';
		}
		else {
		    $finished_pair = 'true';
		}
	    }
	    $output .= '_';
	    $lastScript = '_';
		
	}
	else
	{
	    print "ERROR in webtexParseTensorArguments";
	    
	}


    
    }	#until $scripts eq '';

    $output .= $scripts;

    if ($finished_pair eq 'false') {
	if ($lastScript eq '^') { $output .= '_{}';}
	else { $output .= '^{}'; }
    }

    return substr($output,2);	# Cut off initial {}
}



1;



# WebTeX commands which currently are not implemented:  (as of 8/7/97)
# 
# \_ and \^  Preceding subscripts and superscripts
# Also, LaTeX does not organize tensor scripts in the same way as 
# WebTeX.  A sophisticated WebTeX tensor will break in LaTeX, which expects
# more organization with braces.  These may be easier to deal with when 
# WebTeX scripts are revised.
# 
# Many array options are also not implemented yet.
# 
# Array options 
#   \align{r#n}  Align at a row n
#     This could be done by counting the number of rows and then computing
#     the distance to lift/lower with \raisebox.
# 
#   \equalrows and \equalcolumns
#     There may be a way to get LaTeX to find the width (height) of the 
#     largest column (row) and then force all columns to be that wide.
#     This may be difficult enough that it is wiser to create an environment
#     where we have more control over the width of these amounts.
# 
# Row options
#   \align   Align rows vertically (center,top,bottom)
#     This may be possible with \raisebox, although it is not clear 
#     how to do the computation.
# 
# Cell options
#   \rowspan
#     The package 'multirow', described in The LaTeX Companion, pp. 135-6,
#     is able to do this.  The same page shows a way to use \raisebox to
#     get the same effect after some computation
# 
#   \rowalign
#     Since LaTeX likes to give column alignments at the top of the array,
#     it does not appear that there are any commands that allow this to be
#     changed for individual entries.  
#     According to "A plain TeX Primer", horizontal alignment may be achieved
#     by using \hfill to push the matrix entry to one side or the other.
#     Don't know if there is any way to change vertical alignment.
#
#   \colspan 
#     Now works correctly. WebTeX expects the extra &'s to remain,
#     so they are removed for latex.
# 
# Macros are not implemented yet.  They may have to wait until the form 
# of WebTeX macros is settled.
# 
# 
# 
# WebTeX conversions that could be improved:
# 
# \widecheck is currently changed to \check, which does not change size
# to be as big as the argument.
# 
# \binom is changed to an array.  There may be an easier way to do it.
# Unfortunately, the amstex package's \binom also puts parentheses around
# the expression, so we can't use that.
