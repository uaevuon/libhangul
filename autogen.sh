#!/bin/sh


test -f ChangeLog || touch ChangeLog
test -f config.rpath || touch config.rpath

aclocal $ACLOCAL_AMFLAGS
autoreconf --force --install
autoheader
automake --add-missing --copy
autoconf
