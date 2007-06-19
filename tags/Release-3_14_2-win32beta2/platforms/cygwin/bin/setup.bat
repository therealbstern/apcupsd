rem 
rem  perform installation of apcupsd on Win32 systems
rem
c:
cd \
mkdir tmp
cd c:\apcupsd\bin
path=c:\apcupsd\bin
umount --remove-all-mounts
mount -f -s c:\ /
apcupsd /install
