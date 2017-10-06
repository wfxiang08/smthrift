dnl $Id$
dnl config.m4 for extension foolsock

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(smthrift, for smthrift support,
Make sure that the comment is aligned:
[  --with-smthrift             Include smthrift support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(smthrift, whether to enable smthrift support,
dnl Make sure that the comment is aligned:
dnl [  --enable-smthrift           Enable smthrift support])

if test "$PHP_SMTHRIFT" != "no"; then
  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY_WITH_PATH(stdc++, "", SMTHRIFT_SHARED_LIBADD)
  CXXFLAGS="$CXXFLAGS -std=c++11"

  dnl # --with-smthrift -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/smthrift.h"  # you most likely want to change this
  dnl if test -r $PHP_SMTHRIFT/$SEARCH_FOR; then # path given as parameter
  dnl   SMTHRIFT_DIR=$PHP_SMTHRIFT
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for smthrift files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       SMTHRIFT_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$SMTHRIFT_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the smthrift distribution])
  dnl fi

  dnl # --with-smthrift -> add include path
  dnl PHP_ADD_INCLUDE($SMTHRIFT_DIR/include)

  dnl # --with-smthrift -> check for lib and symbol presence
  dnl LIBNAME=smthrift # you may want to change this
  dnl LIBSYMBOL=smthrift # you most likely want to change this

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $SMTHRIFT_DIR/lib, SMTHRIFT_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_SMTHRIFTLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong smthrift lib version or lib not found])
  dnl ],[
  dnl   -L$SMTHRIFT_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(SMTHRIFT_SHARED_LIBADD)

  PHP_NEW_EXTENSION(smthrift,php_thrift_protocol.cpp smthrift.c, $ext_shared)
fi

if test -z "$PHP_DEBUG"; then
        AC_ARG_ENABLE(debug,
                [--enable-debg  compile with debugging system],
                [PHP_DEBUG=$enableval], [PHP_DEBUG=no]
        )
fi