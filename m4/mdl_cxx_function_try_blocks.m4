dnl MDL_CXX_FUNCTION_TRY_BLOCKS
AC_DEFUN([MDL_CXX_FUNCTION_TRY_BLOCKS],
[
AC_REQUIRE([AC_PROG_CXX])
AC_MSG_CHECKING(whether ${CXX} supports function try blocks)
AC_CACHE_VAL(mdl_cv_have_function_try_blocks,
[
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([void foo() try{} catch( ... ){}],
 [foo();],
 mdl_cv_have_function_try_blocks=yes,
 mdl_cv_have_function_try_blocks=no)
 AC_LANG_RESTORE
])
AC_MSG_RESULT($mdl_cv_have_function_try_blocks)
if test "$mdl_cv_have_function_try_blocks" = no; then
AC_DEFINE([HAVE_NO_FUNCTION_TRY_BLOCKS], [], [Does not support try/catch blocks])
fi])
