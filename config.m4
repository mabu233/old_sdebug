PHP_ARG_ENABLE(sdebug, whether to enable sdebug support,
dnl Make sure that the comment is aligned:
[  --enable-sdebug          Enable sdebug support], no)

PHP_ADD_INCLUDE(/usr/include/libxml2)
if test "$PHP_SDEBUG" != "no"; then
  CFLAGS="$CFLAGS -std=c99"
  AC_DEFINE(HAVE_SDEBUG, 1, [ Have sdebug support ])
  PHP_NEW_EXTENSION(sdebug, sdebug.c dbgp.c sdebug_xml.c, $ext_shared)
fi
