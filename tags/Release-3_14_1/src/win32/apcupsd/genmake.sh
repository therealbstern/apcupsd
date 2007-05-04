rm -f apcupsd.mak
sed -e 's/^\(.*\)\\\(.*\)$/FILENAME=\2\nSOURCE=\1\\\2.cpp\n@@OBJMAKIN@@\n/g' filelist > objtargets1.tmp
sed -e '/@@OBJMAKIN@@/r apcupsd-obj.mak.in' -e '/@@OBJMAKIN@@/d' objtargets1.tmp > objtargets.tmp
sed -e 's/^\(.*\)\\\(.*\)$/\t-@erase "\$(INTDIR)\\\2.obj"/g' filelist > relclean.tmp
sed -e 's/^\(.*\)\\\(.*\)$/\t"\$(INTDIR)\\\2.obj" \\/g' filelist > relobjs.tmp
sed -e 's/^\(.*\)\\\(.*\)$/\t-@erase "\$(INTDIR)\\\2.obj\n\t-@erase "\$(INTDIR)\\\2.sbr"/g' filelist > debclean.tmp
sed -e 's/^\(.*\)\\\(.*\)$/\t"\$(INTDIR)\\\2.obj" \\/g' filelist > debobjs.tmp
sed -e 's/^\(.*\)\\\(.*\)$/\t"\$(INTDIR)\\\2.sbr" \\/g' filelist > debsbrs.tmp
sed     -e '/@@OBJTARGETS@@/r objtargets.tmp' -e '/@@OBJTARGETS@@/d' \
        -e '/@@REL-CLEAN@@/r relclean.tmp' -e '/@@REL-CLEAN@@/d' \
        -e '/@@REL-OBJS@@/r relobjs.tmp' -e '/@@REL-OBJS@@/d' \
        -e '/@@DEB-CLEAN@@/r debclean.tmp' -e '/@@DEB-CLEAN@@/d' \
        -e '/@@DEB-OBJS@@/r debobjs.tmp' -e '/@@DEB-OBJS@@/d' \
        -e '/@@DEB-SBRS@@/r debsbrs.tmp' -e '/@@DEB-SBRS@@/d' \
        apcupsd.mak.in > apcupsd.mak
rm *.tmp
