dnl dps_snprintf_oflow
AC_DEFUN([dps_snprintf_oflow],
	[AC_MSG_CHECKING(whether snprintf ignores n)
	AC_CACHE_VAL(dps_cv_snprintf_bug,
	  [AC_TRY_RUN(
			changequote(<<, >>)dnl
			<<#include <stdio.h>
			
			#ifndef HAVE_SNPRINTF
			#ifdef HAVE_VSNPRINTF
			#include "vsnprintf.h"
			#else /* not HAVE_VSNPRINTF */
			#include "vsnprintf.c"
			#endif /* HAVE_VSNPRINTF */
			#endif /* HAVE_SNPRINTF */
			
			int main(void)
			{
			char ovbuf[7];
			int i;
			for (i=0; i<7; i++) ovbuf[i]='x';
			snprintf(ovbuf, 4,"foo%s", "bar");
			if (ovbuf[5]!='x') exit(1);
			snprintf(ovbuf, 4,"foo%d", 666);
			if (ovbuf[5]!='x') exit(1);
			exit(0);
			} >>
			changequote([, ]), dps_cv_snprintf_bug=0, dps_cv_snprintf_bug=1,
		  dps_cv_snprintf_bug=2
		)
	])
	if test $dps_cv_snprintf_bug -eq 0; then
	  AC_MSG_RESULT([no, snprintf is ok])
	elif test $dps_cv_snprintf_bug -eq 1; then
	  AC_MSG_RESULT([yes, snprintf is broken])
	  AC_DEFINE([HAVE_SNPRINTF_BUG],[][Defined if the snprintf overflow test fails])
	else
	  AC_MSG_RESULT([unknown, assuming yes])
	  AC_DEFINE([HAVE_SNPRINTF_BUG],[],[Defined if the snprintf overflow test fails])
	fi
])
