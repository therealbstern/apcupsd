#!/bin/sh

#
# rc.config apcupsd setup
#
# Copyright (C) 2000 Riccardo Facchetti

RCCONFIG=/etc/rc.config

function do_install_apcupsd() {
	echo "Installing and activating apcupsd at startup."
	cat >> $RCCONFIG << EOF
# Start apcupsd
START_APCUPSD=yes
EOF
}

function do_activate_apcupsd() {
	echo "Activating apcupsd at startup."
	cp $RCCONFIG $RCCONFIG.bak
	cat $RCCONFIG|sed -e 's|START_APCUPSD=no|START_APCUPSD=yes|g'>$RCCONFIG.tmp
	mv $RCCONFIG.tmp $RCCONFIG
}
function do_deactivate_apcupsd() {
	echo "Deactivating apcupsd at startup."
	cp $RCCONFIG $RCCONFIG.bak
	cat $RCCONFIG|sed -e 's|START_APCUPSD=yes|START_APCUPSD=no|g'>$RCCONFIG.tmp
	mv $RCCONFIG.tmp $RCCONFIG
}

function do_erase_apcupsd() {
	echo "Erasing apcupsd at startup."
	cp $RCCONFIG $RCCONFIG.bak
	grep -v "$LINE1" $RCCONFIG > $RCCONFIG.tmp
	mv $RCCONFIG.tmp $RCCONFIG
	grep -v "$LINE2" $RCCONFIG > $RCCONFIG.tmp
	mv $RCCONFIG.tmp $RCCONFIG
}

function erase_apcupsd() {
	LINE1=`grep "# Start apcupsd" $RCCONFIG`
	LINE2=`grep "START_APCUPSD=" $RCCONFIG`

	if [ -z "$LINE1" -o -z "$LINE2" ]
	then
		echo "Can not find apcupsd start directives in $RCCONFIG"
	else
		do_erase_apcupsd
	fi
}

function activate_apcupsd() {
	if [ -z "$APCPRESENT" ]
	then
		do_install_apcupsd
	else
		if [ "$APCPRESENT" = "START_APCUPSD=yes" ]
		then
			echo "apcupsd already active at startup."
		else
			if [ ! "$APCPRESENT" = "START_APCUPSD=no" ]
			then
				erase_apcupsd
			fi
			do_activate_apcupsd
		fi
	fi
}

function deactivate_apcupsd() {
	if [ -z "$APCPRESENT" ]
	then
		echo "apcupsd is not installed in $RCCONFIG"
	else
		if [ "$APCPRESENT" = "START_APCUPSD=yes" ]
		then
			do_deactivate_apcupsd
		else
			echo "apcupsd is already inactive at startup."
		fi
	fi
}

if [ ! -f $RCCONFIG ]
then
	echo "Sorry seem that you don't have the $RCCONFIG file."
	echo "You may need to check your SuSE installation as this is"
	echo "very dangerous."
	exit 1
fi

APCPRESENT=`grep "^START_APCUPSD" $RCCONFIG`

if [ "$1" = "install" ]
then
	activate_apcupsd
	exit 0
fi

if [ "$1" = "uninstall" ]
then
	deactivate_apcupsd
	exit 0
fi

if [ "$1" = "erase" ]
then
	erase_apcupsd
	exit 0
fi
