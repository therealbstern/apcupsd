#!/bin/bash
#
#  inst.sh
#  Caldera-specific install stuff for apcupsd
#
#  John Pinner (john@clocksoft.com).  3 Jan 2000.
#
#  This script uses some stuff from the SuSe apcupsd install scripts
#  by Riccardo Facchetti.  Thank you Riccardo.
#

MAGICWORD="XXX APCUPSD - DO NOT EDIT XXX"
APCCONTROL="/etc/apcupsd/apccontrol killpower"

HALTFILE=/etc/rc.d/init.d/halt

# Do it with functions

Install() {

#  See if we have halt.local facility in halt script
grep -q halt.local $HALTFILE

if [ $? != 0 ]
  then
    # No - add it
    mv $HALTFILE $HALTFILE.0
    cat $HALTFILE.0 | sed -e "/remount/r halt.local.text" > $HALTFILE
    chmod 755 $HALTFILE
fi

# Comment out any UPS stuff in inittab
cat /etc/inittab | sed -f sed.inittab.script >/tmp/sed.$$
cmp -s /etc/inittab /tmp/sed.$$
if [ $? = 1 ]
  then
    # We have commented out Caldera ups stuff - install the new inittab
    mv /etc/inittab /etc/inittab.0
    mv /tmp/sed.$$ /etc/inittab
    chmod 644 /etc/inittab
    telinit q
  else
    rm -f /tmp/sed.$$
fi

# Check for halt.local
if [ ! -f /etc/rc.d/halt.local ]
  then
    # Not there, create a minimal halt.local
    cat <<EOF >/etc/rc.d/halt.local
# halt.local - added by apcupsd install
#              This is executed as an in-line script by halt
#              after all file systems have been unmounted.
#
EOF
    chmod 755 /etc/rc.d/halt.local
fi

# Has apcupsd stuff already been installed in halt.local?
grep -q "$MAGICWORD" /etc/rc.d/halt.local

if [ $? != 0 ]
  then
    # No, put it in
    cat <<EOF >> /etc/rc.d/halt.local
#                                                   # $MAGICWORD
# Added automatically by apcupsd installation       # $MAGICWORD
if [ -f /etc/apcupsd/powerfail ]; then              # $MAGICWORD
    echo "Power down detected while shutting down." # $MAGICWORD
    echo "Preparing UPS to power shutdown."         # $MAGICWORD
    $APCCONTROL                                     # $MAGICWORD
fi                                                  # $MAGICWORD
EOF
fi
}  # end of Install function


UnInstall() {

# No need to uninstall halt.local modifications to halt, or undo inittab changes

if [ -f /etc/rc.d/halt.local ]
  then
    grep -q "$MAGICWORD" /etc/rd.d/halt.local
    if [ $? = 0 ]
      then
        # Remove the apcupsd stuff
        grep -v "$MAGICWORD" /etc/rc.d/halt.local >/tmp/halt.local.$$
        rm /etc/rc.d/halt.local
        mv /tmp/halt.local.$$ /etc/rc.d/halt.local
        chmod 755 /etc/rc.d/halt.local
    fi
fi
}  # end of UnInstall function


case "$1" in
    install ) Install
              ;;
  uninstall ) UnInstall
              ;;
          * ) echo -e "\n\07usage: $0 install | uninstall\n"
              ;;
esac

#  eof inst.sh
