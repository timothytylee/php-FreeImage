dnl $Id: config.m4,v 1.3 2004/07/22 13:32:54 wenlong Exp $
dnl config.m4 for extension freeimage

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(freeimage, for freeimage support,
[  --with-freeimage             Include freeimage support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(freeimage, whether to enable freeimage support,
dnl Make sure that the comment is aligned:
dnl [  --enable-freeimage           Enable freeimage support])


if test "$PHP_FREEIMAGE" != "no"; then
  dnl # --with-freeimage -> check with-path
  SEARCH_PATH="/usr /usr/local"
  SEARCH_FOR="/include/FreeImage.h"
  if test -r $PHP_FREEIMAGE/$SEARCH_FOR; then # path given as parameter
     FREEIMAGE_DIR=$PHP_FREEIMAGE
  else # search default path list
     AC_MSG_CHECKING([for freeimage files in default path])
     for i in $SEARCH_PATH ; do
       if test -r $i/$SEARCH_FOR; then
         FREEIMAGE_DIR=$i
         AC_MSG_RESULT(found in $i)
       fi
     done
  fi

  if test -z "$FREEIMAGE_DIR"; then
    AC_MSG_RESULT([not found])
    AC_MSG_ERROR([Please reinstall the freeimage distribution])
  fi

  AC_LANG_CPLUSPLUS
  PHP_REQUIRE_CXX

  dnl # --with-freeimage -> add include path
  PHP_ADD_INCLUDE($FREEIMAGE_DIR/include)

  dnl # --with-freeimage -> check for lib and symbol presence
  LIBNAME=freeimage
  LIBSYMBOL=FreeImage_GetVersion

  PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  [
    old_LIBS="$LIBS"
    LIBS="$LIBS -lstdc++"
    PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $FREEIMAGE_DIR/lib, FREEIMAGE_SHARED_LIBADD)
    LIBS="$old_LIBS"
    AC_DEFINE(HAVE_FREEIMAGELIB,1,[ ])
  ],[
    AC_MSG_ERROR([wrong freeimage lib version or lib not found])
  ],[
    -L$FREEIMAGE_DIR/lib -lm -ldl
  ])

  PHP_SUBST(FREEIMAGE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(freeimage, freeimage.c freeimage_io.c, $ext_shared)
fi