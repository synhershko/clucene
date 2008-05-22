dnl dps_static_const
AC_DEFUN([dps_static_const],
    [AC_MSG_CHECKING(whether ${CXX} supports static const definitions in classes)
     AC_CACHE_VAL(dps_static_const_type,
		[ 
		 AC_LANG_SAVE
		 AC_LANG_CPLUSPLUS
		 
		 AC_TRY_RUN([class x{public: static const int SCI=55; }; int main(){ x a; if ( a.SCI!=55 ) throw "err"; }],
			 dps_static_const_type=1,
			 dps_static_const_type=0,
			 dps_static_const_type=0)
		
		 if test $dps_static_const_type -eq 0; then
			 AC_TRY_RUN([class x{public: enum{ SCI=55 }; }; int main(){ x a; if ( a.SCI!=55 ) throw "err"; }],
				 dps_static_const_type=2,
				 dps_static_const_type=0,
				 dps_static_const_type=0)
		 fi
		 
		 AC_LANG_RESTORE
		])
    if test $dps_static_const_type -eq 1; then
      AC_MSG_RESULT([yes])
      AC_DEFINE([LUCENE_STATIC_CONSTANT_SYNTAX], 1, [How to define a static const in a class])
    elif test $dps_static_const_type -eq 2; then
      AC_MSG_RESULT([yes])
      AC_DEFINE([LUCENE_STATIC_CONSTANT_SYNTAX], 2, [How to define a static const in a class])
    else
      AC_MSG_ERROR([Cannot figure out how to write static consts in classes. Check the m4 script or upgrade your compiler])
    fi

])