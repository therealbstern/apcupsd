##########################################################################
#
#
#  local.tag
# 
#  These variables may be customized to correspond to the locations
#  of various files in your system.
#
##########################################################################

###
###  IMPORTANT!  The path of perl given in the first line of the 
###              file html2latex MUST be the path of perl on your system!
###

# The location of the directory containing these html2latex files:

#$h2llocation = '/u/schaefer/bin/GChtml2latex';
$h2llocation = '/afs/rpi.edu/dept/acs/rpinfo/filters/GChtml2latex';

# The following variables will be used in html2text-psIMG.tag:
#
#
#  This is the command to convert .gif files to .eps files.  "%in"
#  will be replaced by the input file name and "%out" by the output
#  file name.  You can use any image conversion program, but in the 
#  command below the "%in" and "%out" must be placed where the command
#  expects the input and output files.
#
$htmlScale = "75%";
#  $htmlConvert = "convert -geometry $htmlScale %in %out";
$htmlConvert = "convert -scale 0.75 %in %out";
@htmlImageSizeMax = (5,9); # (width,length) in inches

#
#  This is the name of your main web server.
#
#$htmlWebServer = "http://www.geom.umn.edu";
$htmlWebServer = "http://www.rpi.edu";

#
#  This is where the main web tree begins.
#  The value will replace the name of the web server when it appears
#  in <IMG> tags.
#  (<IMG SRC="http://www.geom.umn.edu/a.jpg"> will be found in 
#   /u/www/root/a.jpg)
#
#$htmlWebRoot = "/u/www/root";
$htmlWebRoot = "/campus/rpi/rpinfo/public_html";

#
#  The is the directory where user's home directories are found.
#  (<IMG SRC="~jones/a.jpg"> will be found in /u/jones/public_html/a.jpg)
#
#$htmlUserRoot = "/u";
$htmlUserRoot = "/home";

#
#  This is the name of the place in the user's home directory 
#  where the user's html files are stored.
#
$htmlUserHTML = "public_html";

#
#  This is the directory where the document originally came from.
#  (the picture files will be looked for here).  This can be specified
#  on the command line with the -home flag.
#
$htmlWebHome = ".";

#
#  The maximum width of a line of text for the output file.
#  Increase this variable if there are too many instances where line breaks
#  are placed in the middle of LaTeX commands.
#
$htmlWidth = 78;
