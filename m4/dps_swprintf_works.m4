dnl dps_swprintf_works
AC_DEFUN([dps_swprintf_works],
  [AC_MSG_CHECKING(whether swprintf is broken)
	AC_CACHE_VAL(dps_cv_swprintf_works,
		[ AC_TRY_RUN(
			changequote(<<, >>)dnl
			<<#include <stdio.h>
			#include <wchar.h>
			
			int main(void)
			{
			wchar_t buf[5];
			swprintf(buf,5,L"%s",L"foo");
			if ( wcslen(buf) != 3 )
				exit(1);
			exit(0);
			} >>
			changequote([, ]), dps_cv_swprintf_works=0, dps_cv_swprintf_works=1,
			dps_cv_swprintf_works=2
		)]
	)
	if test $dps_cv_swprintf_works -eq 0; then
	  AC_MSG_RESULT([no, swprintf is ok])
	elif test $dps_cv_swprintf_works -eq 1; then
	  AC_MSG_RESULT([yes, swprintf is broken])
	  AC_DEFINE([HAVE_SWPRINTF_BUG],[],[Defined if the swprintf test fails])
	else
	  AC_MSG_RESULT([unknown, assuming yes])
	  AC_DEFINE([HAVE_SWPRINTF_BUG],[],[Defined if the swprintf test fails])
	fi
])
