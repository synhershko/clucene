dnl AC_CXX_HAVE_WCTYPE_H
AC_DEFUN([AC_CXX_HAVE_WCTYPE_H],
[AC_CACHE_CHECK(for a functioning wctype.h header,
ac_cv_cxx_have_wctype_h,
[AC_REQUIRE([AC_CXX_NAMESPACES])
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_RUN([#include <wctype.h>
 int main(void){ int x=iswalnum(100); return 0;}],
 ac_cv_cxx_have_wctype_h=yes, ac_cv_cxx_have_wctype_h=no, ac_cv_cxx_have_wctype_h=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_have_wctype_h" = yes; then
  AC_DEFINE(HAVE_WCTYPE_H,,[Define to 1 if you have a functioning <wchar.h> header file.])
fi
])
