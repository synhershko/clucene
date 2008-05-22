dnl AC_DEFINE_INTEGER_BITS
AC_DEFUN([AC_DEFINE_INTEGER_BITS],
[m4_define([ac_datatype_bits], [m4_translit($1, [a-zA-Z_])])
m4_define([ac_datatype_bytes], [m4_eval(ac_datatype_bits/8)])
AC_CHECK_TYPE($1, ,
 [
  AC_MSG_NOTICE([trying to find a suitable ]ac_datatype_bytes[-byte replacement for $1])
  $1=no
  find_$1 ()
  {
    _AC_DEFINE_INTEGER_BITS_HELPER($@)
    :
  }
  find_$1
  AC_DEFINE_UNQUOTED($1, $$1,
    [If not already defined, then define as a datatype of *exactly* ]ac_datatype_bits[ bits.])
 ])
])

dnl Iterate over arguments $2..$N, trying to find a good match for $1.
m4_define([_AC_DEFINE_INTEGER_BITS_HELPER],
[ifelse($2, , ,
 [m4_define([ac_datatype_bits], [m4_translit($1, [a-zA-Z_])])
  m4_define([ac_datatype_bytes], [m4_eval(ac_datatype_bits/8)])
  AC_CHECK_SIZEOF($2)
  if test "$AS_TR_SH(ac_cv_sizeof_$2)" -eq ac_datatype_bytes; then
    $1="$2"
    return
  fi
  _AC_DEFINE_INTEGER_BITS_HELPER($1, m4_shift(m4_shift($@)))
 ])
])
