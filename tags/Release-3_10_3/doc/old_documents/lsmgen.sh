#!/usr/bin/perl
#
# lsmgen.sh -- generate current LSM for apcupsd
#
$version = $ARGV[0];
$size = substr($ARGV[1], 0, 3);

@months
  = ('JAN','FEB','MAR','APR','MAY','JUN','JUL','AUG','SEP','OCT','NOV','DEC');

my($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime(time);
$month = $months[$mon];

print <<EOF;
Begin3
Title:		apcupsd
Version:	$version
Entered-date:	${mday}${month}${year}
Description:	UPS power management under Linux for APCC Products.
		BackUPS, BackUPS Pro, SmartUPS V/S, and SmartUPS(NET/RM)
		success. MatrixUPS and ShareUPS testing stage.
		SmartUPS V/S is untested, but should be okay.
		It allows your computer/server to run during power problems
		for a specified length of time or the life of the batteries
		in your BackUPS, BackUPS Pro, SmartUPS v/s, or SmartUPS, and
		then properly executes a controlled shutdown during an
		extended power failure.
		The following APC cables are supported fully:
			940-0020B,
			940-0024B, 940-0024C, 940-0024G,
			940-1524C,
			940-0095A, 940-0095C,
			940-0023A(Last resort, please pick another.)
		Successful tests on BackUPS 400/600, BackUPS Pro 420/650/1400,
		SmartUPS v/s, and SmartUPS SU700RM/SU1250/SU1400RM w/ SMNP
		PowerNET card. Current testing with ShareUPS/SU3000 Matrix.
		EtherUPS or NetUPS is clean and works but needs slave
		disconnect/reconnect polish.
		APCaccess is the IPC management tool for "apcupsd".
		APCaccess is used to manage slaves locally.
Keywords:	power, ups, BackUPS, BackUPS Pro, SmartUPS V/S, SmartUPS,
		MatrixUPS, PowerNET, ShareUPS, (EtherUPS or NetUPS) (C)
Required:	Linux 2.0.X, gcc 2.7.2
		Linux 2.0.33, gcc 2.7.2.3, [ libc6|libc.so.2.0.6|glibc2 ]
Author:		Andre Hedrick <hedrick\@astro.dyer.vanderbilt.edu>
Maintained-by:	Andre Hedrick
Linux Flavors:	Slackware 3.X, S.u.S.e 5.X, UNIFIX Linux 2.0,
		RedHat [3|4|5].X, Debian 1.X.X
Primary-site:	sunsite.unc.edu/pub/Linux/system/ups	
		${size}K apcupsd-$version.tar.gz
Alternate-site:	http://www.dyer.vanderbilt.edu/server/apcupsd/
		${size}K apcupsd-$version.tar.gz
Copying-policy:	Package is to remain in original format.
		RPM, DEB, and other all other package managers are forbidden,
		without formal requests.  Addition of a GPL license to the 
		package will be a direct violation of this policy.
		For non-commerial use, without commerial license agreement
		Office of Technology Transfer, Vanderbilt University.
Copyright:	Andre M. Hedrick <hedrick\@astro.dyer.vanderbilt.edu>
Platform:	Linux(ix86)
Other:		S.u.S.e 5.0 distribution donated by S.u.S.e U.S.A.
		ASA Computers, Inc. donated a BackUPS Pro 650 PnP
		<http://www.asacomputers.com/> <kedar\@asacomputers.com>
End
Platforms:	All
Copying-policy:	GPL
End
EOF

