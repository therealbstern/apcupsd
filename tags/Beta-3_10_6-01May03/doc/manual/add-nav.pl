#!/usr/bin/perl
#
#   Add navigation buttons to a list of files.
#      By Kern Sibbald
#
#   This is a very simplistic program. You call it
#      ./add-nav.pl 
#   It assumes:
#      - perl is in /usr/bin/perl (if not, set a link)
#      - there is a file named add-nav.list in the current directory
#      - This list supplies the names of all the html files to which
#        you wish to add buttons.  
#      - The buttons are named back.gif, next.gif and home.gif
#      - The buttons are added two lines before the end of the
#        html file (that means that the last two lines should be 
#           </body>
#           </html>
#        You probably will want the previous line to be:  <hr>
#      - The first file is ASSUMED to be index.html and is not included
#        in the add-nav.list file
#
#   Whew!  There are a lot of assumptions.  In addition, the output
#          when running ./add-nav.pl is not very pretty, but so what,
#          it gets the job done.
#
#   This program needs to be run only once. There after, you run it 
#     only if you add a new file (be sure to add it to add-nav.list),
#     or if you want to re-order the files.
#

#
# check that the file exists and is a normal file
sub chkfile {
   if ( ! -e @_[0] ) {
      die ("File ".@_[0]." does not exist.\n");
   }
   if ( ! -f @_[0] ) {
      die ( "File ".@_[0]." must be a normal file.\n");
   }
}


#
# get rid of any line containing <img src="back.gif"
#                                <img src="next.gif"
#                                <img src="home.gif"
sub clean_file {
   @outf = ();
   chkfile(@_[0]);
   open(HTML, @_[0]);
   while (<HTML>) {
      # strip any old navigation buttons.
      if (! /\<img\ src\=\"back\.gif\"/ &&  
          ! /\<img\ src\=\"next\.gif\"/ &&  
          ! /\<img\ src\=\"home\.gif\"/ ) {
         push @outf, $_;
      }
   }
   close(HTML);
   printf("Cleaned @_[0]\n");
   
}

# ======= begin main program ========

    @navfiles = ();
    open(NAV, "add-nav.list") or die("add-nav.list could not be opened");
    @navfiles = <NAV>;
    close(NAV);

    $n = $#navfiles;
    printf("Number of files is $n\n");
    for ($i=0; $i<=$n; $i++) {
       $f = @navfiles[$i];
       printf("Processing $f\n");
       chomp($f);
       clean_file($f);
       $l = $#outf;
       if ($i == 0) {
          $next = @navfiles[$i+1];
          chomp($next);
          splice(@outf, $l-1, 0, 
"<a href=\"$next\" target=\"_self\"><img src=\"next.gif\" border=0 alt=\"Next\"></a>\n" .
"<a href=\"index.html\"><img src=\"home.gif\" border=0 alt=\"Home\"></a>\n");
          printf("i is zero\n");
       } else {
          if ($i == $n) {
          $prev = @navfiles[$i-1];
          chomp($prev);
          splice(@outf, $l-1, 0, 
"<a href=\"$prev\" target=\"_self\"><img src=\"back.gif\" border=0 alt=\"Back\"></a>\n" .
"<a href=\"index.html\"><img src=\"home.gif\" border=0 alt=\"Home\"></a>\n");
          printf("i is last $n\n");
          } else {
          $next = @navfiles[$i+1];
          chomp($next);
          $prev = @navfiles[$i-1];
          chomp($prev);
          splice(@outf, $l-1, 0, 
"<a href=\"$prev\" target=\"_self\"><img src=\"back.gif\" border=0 alt=\"Back\"></a>\n" .
"<a href=\"$next\" target=\"_self\"><img src=\"next.gif\" border=0 alt=\"Next\"></a>\n" .
"<a href=\"index.html\"><img src=\"home.gif\" border=0 alt=\"Home\"></a>\n");
          }
       }
#      printf("$f\n");

       open OUTF, ">$f";
       foreach $f (@outf) {
           print OUTF $f;
       }
       close OUTF;

    }
exit 0;   
